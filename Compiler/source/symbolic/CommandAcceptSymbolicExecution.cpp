//
// Created by abdul on 10/08/17.
//
#include "../Command.h"
#include "SymbolicExecution.h"
#include "../compile/VarWrappers.h"

using namespace std;

bool AbstractCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe>, bool repeat)
{
    return true;
}

bool InputVarCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat)
{
    SymbolicVariable* found = svs->symbolicVarSet->findVar(getData());
    if (found == nullptr) throw "should be found";
    found->userInput();
    return true;
}

bool PushCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat)
{
    if (calledFunction != nullptr) svs->symbolicStack->pushState(getData());

    else switch(stringType)
    {
        case StringType::ID:
        {
            SymbolicVariable* found = svs->symbolicVarSet->findVar(getData());
            if (found == nullptr)
            {
                svs->error(Reporter::UNDECLARED_USE, "'" + getData() + "' pushed without being declared", getLineNum());
                return false;
            }
            if (!found->isFeasable()) throw "should be feasable";
            else if (!found->isDefined())
            {
                svs->warn(Reporter::UNINITIALISED_USE, "'" + getData() + "' pushed without being defined", getLineNum());
            }

            svs->symbolicStack->pushVar(found);
            break;
        }
        case StringType::DOUBLELIT:
        {
            svs->symbolicStack->pushDouble(stod(getData()));
            break;
        }
        case StringType::STRINGLIT:
        {
            svs->symbolicStack->pushString(getData());
            break;
        }
        default:
            throw "unfamiliar string type";
    };
    return true;
}

bool PrintIndirectCommand::acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs,
                                                   bool repeat)
{
    return toPrint->check(svs.get());
}

bool PopCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat)
{
    if (svs->symbolicStack->isEmpty())
    {
        svs->error(Reporter::BAD_STACK_USE, "Tried to pop empty stack", getLineNum());
        return false;
    }

    if (isEmpty())
    {
        svs->symbolicStack->pop();
        return true;
    }

    if (svs->symbolicStack->peekTopType() == SymbolicStackMemberType::STATE)
    {
        svs->error(Reporter::BAD_STACK_USE, "Tried to pop a state into a variable", getLineNum());
        return false;
    }

    SymbolicVariable* found = svs->symbolicVarSet->findVar(getData());
    if (found == nullptr)
    {
        svs->error(Reporter::UNDECLARED_USE, "'" + getData() + "' popped into without being declared", getLineNum());
        return false;
    }

    unique_ptr<SymbolicVariable> popped = svs->symbolicStack->popVar();
    if (popped->getType() != found->getType())
    {
        svs->error(Reporter::TYPE, "Tried to pop '" + popped->getName() + "' (type " + TypeEnumNames[popped->getType()]
                                   +") into '" + found->getName() + "' (type " + TypeEnumNames[found->getType()] + ")");
        return false;
    }

    popped->setName(found->getName());
    if (!popped->isFeasable()) throw "should be feasable";
    svs->symbolicVarSet->addVar(move(popped));

    return true;
}

bool AssignVarCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat)
{
    SymbolicVariable* svp = svs->symbolicVarSet->findVar(getData());
    if (svp == nullptr)
    {
        svs->error(Reporter::UNDECLARED_USE, "'" + getData() + "' assigned to without being declared", getLineNum());
        return false;
    }

    if (svp->getType() == DOUBLE)
    {
        try
        {
            stod(rhs);
            svp->setConstValue(rhs);
            return true;
        }
        catch (invalid_argument&)
        {
            SymbolicVariable* RHSvar = svs->symbolicVarSet->findVar(rhs);
            if (RHSvar == nullptr)
            {
                svs->error(Reporter::UNDECLARED_USE, "'" + rhs + "' used on rhs without being declared", getLineNum());
                return false;
            }
            else if (RHSvar->getType() != DOUBLE)
            {
                svs->error(Reporter::TYPE, "'" + rhs + "' (type " + TypeEnumNames[RHSvar->getType()] +
                                           ") assigned to double", getLineNum());
                return false;
            }

            unique_ptr<SymbolicDouble> newLHS = make_unique<SymbolicDouble>(RHSvar);
            if (!newLHS->isFeasable()) return false;
            newLHS->setName(getData());
            svs->symbolicVarSet->addVar(move(newLHS));
            return true;
        }
    }
    else svp->setConstValue(rhs);
    svp->define();
    return svp->isFeasable();
}

bool EvaluateExprCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat)
{
    SymbolicVariable* LHS = sef->symbolicVarSet->findVar(getData());
    if (LHS == nullptr)
    {
        sef->error(Reporter::UNDECLARED_USE, "'" + getData() + "' assigned to without being declared", getLineNum());
        return false;
    }
    else if (LHS->getType() != DOUBLE)
    {
        sef->error(Reporter::TYPE, "'" + getData() + "' (type " + TypeEnumNames[LHS->getType()] +
                ") used in arithmetic evaluation", getLineNum());
        return false;
    }

    if (op != MOD && repeat)
    {
        if (!term2.isLit) LHS->userInput();
        else
        {
            double t2 = term2.d;
            SymbolicDouble* sd = static_cast<SymbolicDouble*>(LHS);

            switch (op)
            {
                case ArithOp::MINUS:
                    t2 *= -1;
                case ArithOp::PLUS:
                    if (t2 > 0) sd->removeUpperBound();
                    else if (t2 < 0) sd->removeLowerBound();
                    break;

                case ArithOp::DIV:
                    if (t2 == 0) throw runtime_error("divide by 0");
                    else t2 = 1/t2;
                case ArithOp::MULT:
                    if (abs(t2) > 1)
                    {
                        if (sd->getTUpperBound() > 0) sd->removeUpperBound();
                        else if (sd->getTUpperBound() < 0) sd->removeLowerBound();
                        if (sd->getTLowerBound() < 0) sd->removeLowerBound();
                        else if (sd->getTLowerBound() > 0) sd->removeUpperBound();
                    }
                    else if (abs(t2) < 1)
                    {
                        if (sd->getTLowerBound() > 0) sd->setTLowerBound(0);
                        if (sd->getTUpperBound() < 0) sd->setTUpperBound(0);
                    }

                default:
                    throw runtime_error("Unsupported op");
            }
        }
    }
    else
    {
        //todo make it use consts directly instead of creating vars
        GottenVarPtr<SymbolicDouble> t1(nullptr);
        if (term1.isLit)
        {
            t1.reset(make_unique<SymbolicDouble>("LHSconst", sef->reporter));
            t1->setTConstValue(term1.d);
        }
        else
        {
            auto t1gvp = term1.vg->getSymbolicDouble(sef.get());
            t1.become(t1gvp);
            if (!t1)
            {
                sef->error(Reporter::UNDECLARED_USE, "'" + term1.vg->getFullName() + "' used without being declared", getLineNum());
                return false;
            }
            else if (!t1->isDefined()) sef->warn(Reporter::TYPE, "'" + term1.vg->getFullName() + "' used before definition", getLineNum());

            //else if (t1->getType() != DOUBLE)
            //{
            //    sef->error(Reporter::TYPE, "'" + term1 + "' (type " + TypeEnumNames[t1->getType()] +
            //                               ") used in arithmetic evaluation", getLineNum());
            //   return false;
            //}
        }

        GottenVarPtr<SymbolicDouble> t2(nullptr);
        if (term2.isLit)
        {
            t2.reset(make_unique<SymbolicDouble>("RHSconst", sef->reporter));
            t2->setTConstValue(term2.d);
        }
        else
        {
            auto t2gvp = term2.vg->getSymbolicDouble(sef.get());
            t2.become(t2gvp);
            if (!t2)
            {
                sef->error(Reporter::UNDECLARED_USE, "'" + term2.vg->getFullName() + "' used without being declared", getLineNum());
                return false;
            }
            else if (!t2->isDefined()) sef->warn(Reporter::TYPE, "'" + term2.vg->getFullName() + "' used before definition", getLineNum());

            //else if (t2->getType() != DOUBLE)
            //{
            //    sef->error(Reporter::TYPE, "'" + term2 + "' (type " + TypeEnumNames[t2->getType()] +
            //                               ") used in arithmetic evaluation", getLineNum());
            //   return false;
            //}
        }
        
        unique_ptr<SymbolicDouble> result = make_unique<SymbolicDouble>(t1.get());
        SymbolicDouble t2copy(t2.get());

        switch (op)
        {
            case PLUS:
                result->addSymbolicDouble(t2copy);
                break;
            case MULT:
                result->multSymbolicDouble(t2copy);
                break;
            case MINUS:
                t2copy.multConst(-1);
                result->addSymbolicDouble(t2copy);
                break;
            case DIV:
                result->divSymbolicDouble(t2copy);
                break;
            case MOD:
                result->modSymbolicDouble(t2copy);
                break;
            default:
                throw runtime_error("Bitwise operations not supported");
        }
        result->setName(LHS->getName());
        result->define();
        sef->symbolicVarSet->addVar(move(result));
    }
    return sef->isFeasable();
}

bool DeclareVarCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat)
{
    if (vt == STRING) svs->symbolicVarSet->addVar(make_unique<SymbolicString>(getData(), svs->reporter));
    else if (vt == DOUBLE) svs->symbolicVarSet->addVar(make_unique<SymbolicDouble>(getData(), svs->reporter));
    else throw runtime_error("Bad type in DeclareVarCommand");
    return true;
}

bool DeclareArrayCommand::acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs,
                                                  bool repeat)
{
    svs->symbolicVarSet->addArray(getData(), make_unique<SymbolicArray>(getData(), size, svs->reporter));
    return true;
}

