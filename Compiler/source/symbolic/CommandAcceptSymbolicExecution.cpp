//
// Created by abdul on 10/08/17.
//
#include "../Command.h"
#include "SymbolicExecution.h"

using namespace std;

typedef shared_ptr<SymbolicVariableTemplate<string>> StringTemplatePtr;
typedef shared_ptr<SymbolicVariableTemplate<double>> DoubleTemplatePtr;

bool AbstractCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs)
{
    return true;
}

bool InputVarCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs)
{
    SymbolicVariable* found = svs->symbolicVarSet->findVar(getData());
    if (found == nullptr) throw "should be found";
    found->userInput();
    return true;
}

bool PushCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs)
{
    if (pushType == PUSHSTATE) svs->currentStack->push(getData());

    else
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
        svs->currentStack->push(found);
    }
    return true;
}

bool PopCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs)
{
    if (svs->currentStack->getTopType() != VAR)
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

    unique_ptr<SymbolicVariable> popped = svs->currentStack->popVar();
    if (popped->getType() != found->getType())
    {
        svs->error(Reporter::TYPE, "Tried to pop '" + popped->getName() + "' (type " + TypeEnumNames[popped->getType()]
                                   +") into '" + found->getName() + "' (type " + TypeEnumNames[found->getType()] + ")");
        return false;
    }

    popped->setName(found->getName());
    if (!popped->isFeasable()) throw "should be feasable";
    svs->symbolicVarSet->defineVar(move(popped));
    return true;
}

bool AssignVarCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs)
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
            stod(RHS);
            svp->setConstValue(RHS);
            return true;
        }
        catch (invalid_argument e)
        {
            SymbolicVariable* RHS = svs->symbolicVarSet->findVar(getData());
            if (RHS == nullptr)
            {
                svs->error(Reporter::UNDECLARED_USE, "'" + getData() + "' assigned to without being declared", getLineNum());
                return false;
            }
            else if (RHS->getType() != DOUBLE)
            {
                svs->error(Reporter::TYPE, "'" + getData() + "' (type " + TypeEnumNames[RHS->getType()] +
                                           ") assigned to double", getLineNum());
                return false;
            }
            else svs->symbolicVarSet->defineVar(make_unique<SymbolicDouble>(RHS));
        }
    }
    else svp->setConstValue(RHS);
    svp->define();
    if (!svp->isFeasable()) throw "should be feasable";
    return true;
}

bool EvaluateExprCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs)
{
    SymbolicVariable* LHS = svs->symbolicVarSet->findVar(getData());
    if (LHS == nullptr)
    {
        svs->error(Reporter::UNDECLARED_USE, "'" + getData() + "' assigned to without being declared", getLineNum());
        return false;
    }
    else if (LHS->getType() != DOUBLE)
    {
        svs->error(Reporter::TYPE, "'" + getData() + "' (type " + TypeEnumNames[LHS->getType()] +
                ") used in arithmetic evaluation", getLineNum());
        return false;
    }

    //todo make it use consts directly instead of creating vars
    bool deletet1 = true;
    SymbolicVariable* t1;
    try
    {
        stod(term1);
        t1 = new SymbolicDouble("LHSconst", svs->reporter); //if we get here it's a double
        t1->setConstValue(term1);
    }
    catch (invalid_argument e)
    {
        t1 = svs->symbolicVarSet->findVar(term1);
        if (t1 == nullptr)
        {
            svs->error(Reporter::UNDECLARED_USE, "'" + term1 + "' used without being declared", getLineNum());
            return false;
        }
        else if (t1->getType() != DOUBLE)
        {
            svs->error(Reporter::TYPE, "'" + term1 + "' (type " + TypeEnumNames[t1->getType()] +
                                       ") used in arithmetic evaluation", getLineNum());
            return false;
        }
        else if (!t1->isDefined()) svs->warn(Reporter::TYPE, "'" + term1 + "' used before definition", getLineNum());
        deletet1 = false;
    }

    bool deletet2 = true;
    SymbolicVariable* t2;
    try
    {
        stod(term2);
        t2 = new SymbolicDouble("RHSconst", svs->reporter);
        t2->setConstValue(term2);
    }
    catch (invalid_argument)
    {
        t2 = svs->symbolicVarSet->findVar(term2);
        if (t2 == nullptr)
        {
            svs->error(Reporter::UNDECLARED_USE, "'" + term2 + "' used without being declared", getLineNum());
            return false;
        }
        else if (t2->getType() != DOUBLE)
        {
            svs->error(Reporter::TYPE, "'" + term2 + "' (type " + TypeEnumNames[t2->getType()] +
                                       ") used in arithmetic evaluation", getLineNum());
            return false;
        }
        else if (!t2->isDefined()) svs->warn(Reporter::TYPE, "'" + term2 + "' used before definition", getLineNum());

        deletet2 = false;
    }

    unique_ptr<SymbolicDouble> result = make_unique<SymbolicDouble>(t1);
    SymbolicDouble t2copy(t2);

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
    svs->symbolicVarSet->defineVar(move(result));
    return svs->isFeasable();
}

bool DeclareVarCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs)
{
    if (vt == STRING) svs->symbolicVarSet->defineVar(make_unique<SymbolicString>(getData(), svs->reporter));
    else if (vt == DOUBLE) svs->symbolicVarSet->defineVar(make_unique<SymbolicDouble>(getData(), svs->reporter));
    else throw runtime_error("Bad type in DeclareVarCommand");
    return true;
}

