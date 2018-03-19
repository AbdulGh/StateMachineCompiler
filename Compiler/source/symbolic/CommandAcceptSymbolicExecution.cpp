//
// Created by abdul on 10/08/17.
//
#include "../Command.h"
#include "SymbolicExecution.h"
#include "VarWrappers.h"

using namespace std;

bool AbstractCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe>, bool repeat)
{
    return true;
}

bool InputVarCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat)
{
    vs->nondet(sef.get());
    return true;
}

bool PushCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat)
{
    if (calledFunction != nullptr) sef->symbolicStack->pushState(*atom.getString());

    else switch(stringType)
    {
        case StringType::ID:
        {
            GottenVarPtr<SymbolicVariable> found = atom.getVarWrapper()->getSymbolicVariable(sef.get());
            if (!found->isFeasable()) throw "should be feasable";
            else if (!found->isDefined())
            {
                sef->warn(Reporter::UNINITIALISED_USE, "'" + found->getName() + "' pushed without being defined", getLineNum());
            }

            sef->symbolicStack->pushVar(move(found));
            break;
        }
        case StringType::DOUBLELIT:
        {
            sef->symbolicStack->pushDouble(stod(*atom.getString()));
            break;
        }
        case StringType::STRINGLIT:
        {
            sef->symbolicStack->pushString(*atom.getString());
            break;
        }
        default:
            throw "unfamiliar string type";
    };
    return true;
}

bool PopCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat)
{
    if (sef->symbolicStack->isEmpty())
    {
        sef->error(Reporter::BAD_STACK_USE, "Tried to pop empty stack", getLineNum());
        return false;
    }

    if (isEmpty())
    {
        sef->symbolicStack->pop();
        return true;
    }

    if (sef->symbolicStack->peekTopType() == SymbolicStackMemberType::STATE)
    {
        sef->error(Reporter::BAD_STACK_USE, "Tried to pop a state into a variable", getLineNum());
        return false;
    }


    unique_ptr<SymbolicVariable> popped = sef->symbolicStack->popVar();
    vs->setSymbolicVariable(sef.get(), popped.release());
    return true;
}

bool AssignVarCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat)
{
    if (atom.isHolding())
    {
        GottenVarPtr<SymbolicVariable> svp = atom.getVarWrapper()->getSymbolicVariable(sef.get());
        if (svp->isFeasable()) return false;
        vs->setSymbolicVariable(sef.get(), svp.get());
    }
    else atom.getVarWrapper()->getSymbolicVariable(sef.get())->setConstValue(*atom.getString()); //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa

    return true;
}

bool EvaluateExprCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat)
{
    term1.vg->check(sef.get());
    if (op != MOD && repeat)
    {
        if (!term2.isLit)
        {
            vs->nondet(sef.get());
            term2.vg->check(sef.get());
        }
        else
        {
            double t2 = term2.d;
            GottenVarPtr<SymbolicDouble> sd = vs->getSymbolicDouble(sef.get());

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

            vs->setSymbolicVariable(sef.get(), sd.get());
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
            term1.vg->check(sef.get());
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
            term2.vg->check(sef.get());
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
        vs->setSymbolicVariable(sef.get(), result.get());
    }
    return sef->isFeasable();
}

bool DeclareVarCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat)
{
    if (vt == STRING) sef->symbolicVarSet->addVar(make_unique<SymbolicString>(name, sef->reporter));
    else if (vt == DOUBLE) sef->symbolicVarSet->addVar(make_unique<SymbolicDouble>(name, sef->reporter));
    else throw runtime_error("Bad type in DeclareVarCommand");
    return true;
}

bool DeclareArrayCommand::acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef,
                                                  bool repeat)
{
    sef->symbolicVarSet->addArray(name, make_unique<SymbolicArray>(name, size, sef->reporter));
    return true;
}

