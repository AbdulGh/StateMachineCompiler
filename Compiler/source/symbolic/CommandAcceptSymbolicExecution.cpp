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
    if (vs->check(sef.get()))
    {
        vs->nondet(sef.get());
        return true;
    }
    else return false;
}

bool PushCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat)
{
    if (calledFunction != nullptr) sef->symbolicStack->pushState(state);

    else switch(stringType)
    {
        case StringType::ID:
        {
            if (atom->getVarWrapper()->check(sef.get()))
            {
                GottenVarPtr<SymbolicVariable> found = atom->getVarWrapper()->getSymbolicVariable(sef.get());

                if (!found->isFeasable()) throw std::runtime_error("should be feasable");
                else if (!found->isDefined())
                {
                    sef->warn(Reporter::UNINITIALISED_USE, "'" + found->getName() + "' pushed without being defined", getLineNum());
                }

                sef->symbolicStack->pushVar(move(found));
            }
            else return false;
            break;
        }
        case StringType::DOUBLELIT:
        {
            sef->symbolicStack->pushDouble(stod(*atom->getString()));
            break;
        }
        case StringType::STRINGLIT:
        {
            sef->symbolicStack->pushString(*atom->getString());
            break;
        }
        default:
            throw std::runtime_error("unfamiliar string type");
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
    if (vs->check(sef.get()))
    {
        vs->setSymbolicVariable(sef.get(), popped.get());
        return true;
    }
    else return false;
}

bool AssignVarCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat)
{
    if (!vs->check(sef.get())) return false;
    if (atom.isHolding())
    {
        if (!atom.getVarWrapper()->check(sef.get())) return false;
        GottenVarPtr<SymbolicVariable> svp = atom.getVarWrapper()->getSymbolicVariable(sef.get());
        if (!svp->isFeasable()) return false;
        vs->setSymbolicVariable(sef.get(), svp.get());
    }
    else vs->getSymbolicVariable(sef.get())->setConstValue(*atom.getString());

    return true;
}

bool EvaluateExprCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat)
{
    if (op == AND || op == OR) throw runtime_error("Bitwise operations not supported by verifier");
    else if (op == PLUS || op == MULT)
    {
        auto multConst = [&, this, sef, repeat] (SymbolicDouble* result, double c) -> void
        {
            double absc = abs(c);
            if (absc == 0) result->setTConstValue(0);
            else if (absc == 1) return;


            if (repeat)
            {
                if (absc > 1)
                {
                    result->maxUpperBound();
                    result->minLowerBound();
                    result->multConst(c);
                }
                else
                {
                    if (result->getTLowerBound() > 0) result->setTLowerBound(0);
                    if (result->getTUpperBound() < 0) result->setTUpperBound(0);
                }
            }
            else result->multConst(c);
        };

        auto addConst = [&, this, sef, repeat] (SymbolicDouble* result, double c) -> void
        {
            if (c == 0) return;
            else if (repeat)
            {
                if (c > 0) result->maxUpperBound();
                else result->minLowerBound();
            }
            result->addConst(c);
        };

        if (!term1.isLit && !term2.isLit)
        {
            if (!term1.vg->check(sef.get()) || !term2.vg->check(sef.get())) return false;

            unique_ptr<SymbolicDouble> result = term1.vg->getSymbolicDouble(sef.get())->cloneSD();
            GottenVarPtr<SymbolicDouble> t2 = term2.vg->getSymbolicDouble(sef.get());

            if (result->isDetermined())
            {
                if (op == PLUS) addConst(t2.get(), result->getTConstValue());
                else multConst(t2.get(), result->getTConstValue());
            }
            else if (t2->isDetermined())
            {
                if (op == PLUS) addConst(result.get(), t2->getTConstValue());
                else multConst(result.get(), t2->getTConstValue());
            }
            else
            {
                if (repeat)
                {
                    if (t2->getTUpperBound() > 0) result->maxUpperBound();
                    if (t2->getTLowerBound() < 0) result->minLowerBound();
                }
                else
                {
                    if (op == PLUS)
                    {
                        std::string vsName = vs->getFullName();
                        bool increment = vsName == term1.vg->getFullName() || vsName == term2.vg->getFullName();
                        result->addSymbolicDouble(*t2, increment);
                    }
                    else result->multSymbolicDouble(*t2);
                }
            }

            vs->setSymbolicVariable(sef.get(), result.get());
            return true;
        }
        else
        {
            double constTerm; VarWrapper* varWrapper;
            if (!term1.isLit)
            {
                if (!term1.vg->check(sef.get())) return false;
                varWrapper = term1.vg.get();
                constTerm = term2.d;
            }
            else
            {
                if (!term2.vg->check(sef.get())) return false;
                varWrapper = term2.vg.get();
                constTerm = term1.d;
            }

            unique_ptr<SymbolicDouble> result = varWrapper->getSymbolicDouble(sef.get())->cloneSD();

            if (op == PLUS) addConst(result.get(), constTerm);
            else multConst(result.get(), constTerm);

            vs->setSymbolicVariable(sef.get(), result.get());
            return true;
        }
    }

    else //not commutative
    {
        auto varModVar = [&sef, this] (SymbolicDouble* result, SymbolicDouble* t2v) -> void
        {
            if (t2v->getTLowerBound() <= 0 && t2v->getTUpperBound() >= 0)
            {
                sef->warn(Reporter::ZERODIVISION, "RHS could potentially be 0", getLineNum());
            }

            result->clipTLowerBound(0);
            result->clipTUpperBound(max(abs(t2v->getTLowerBound()), abs(t2v->getTUpperBound())));
        };

        if (!term1.isLit)
        {
            unique_ptr<SymbolicDouble> result = term1.vg->getSymbolicDouble(sef.get())->cloneSD();
            if (repeat)
            {
                double t2c;

                if (!term2.isLit)
                {
                    if (!term2.vg->check(sef.get())) return false;
                    GottenVarPtr<SymbolicDouble> t2v = term2.vg->getSymbolicDouble(sef.get());
                    if (t2v->isDetermined()) t2c = t2v->getTConstValue();
                    else
                    {
                        if (op == MINUS)
                        {
                            if (t2v->getTUpperBound() > 0) result->minLowerBound();
                            else if (t2v->getTLowerBound() < 0) result->maxUpperBound();
                        }
                        else if (op == MOD) varModVar(result.get(), t2v.get());
                        else if (op == DIV) //todo this is wrong if the rhs can be negative
                        {
                            if (t2v->getTLowerBound() <= 0 && t2v->getTUpperBound() >= 0)
                            {
                                sef->warn(Reporter::ZERODIVISION, "RHS could potentially be 0", getLineNum());
                            }

                            double abslb = abs(t2v->getTLowerBound());
                            double absub = abs(t2v->getTUpperBound());
                            double minabs, maxabs;
                            if (abslb > absub)
                            {
                                minabs = absub;
                                maxabs = abslb;
                            }
                            else
                            {
                                minabs = abslb;
                                maxabs = absub;
                            }

                            if (minabs < 1)
                            {
                                if (result->getTLowerBound() > 0)
                                {
                                    result->setTLowerBound(max(result->getTRepeatLowerBound(), 0.0));
                                }

                                if (result->getTUpperBound() < 0)
                                {
                                    result->setTUpperBound(min(result->getTRepeatUpperBound(), 0.0));
                                }
                            }
                            if (maxabs > 1)
                            {
                                result->minLowerBound();
                                result->maxUpperBound();
                            }
                        }
                        else throw runtime_error("Strange op encountered");

                        vs->setSymbolicVariable(sef.get(), result.get());
                        return true;


                        /*unique_ptr<SymbolicDouble> result = term1.vg->getSymbolicDouble(sef.get())->cloneSD();
                        result->maxUpperBound();
                        result->minLowerBound();
                        vs->setSymbolicVariable(sef.get(), result.get());
                        return true;*/
                    }
                }

                if (op == MINUS)
                {
                    if (t2c > 0) result->minLowerBound();
                    else if (t2c < 0) result->maxUpperBound();
                }
                else if (op == MOD) result->modConst(t2c);
                else if (op == DIV)
                {
                    if (t2c == 0)
                    {
                        sef->error(Reporter::ZERODIVISION, "RHS must be zero", getLineNum());
                        return false;
                    }
                    if (t2c < 0)
                    {
                        if (t2c < -1)
                        {
                            double maxAbs = max(abs(result->getTLowerBound()), abs(result->getTUpperBound()));
                            result->setTLowerBound(-maxAbs);
                            result->setTUpperBound(maxAbs);
                        }
                        else
                        {
                            result->minLowerBound();
                            result->maxUpperBound();
                        }
                    }
                }
                else throw runtime_error("Strange op encountered");
                vs->setSymbolicVariable(sef.get(), result.get());
                return true;
            }

            if (!term1.vg->check(sef.get())) return false;
            if (!term2.isLit) //var and var
            {
                if (!term2.vg->check(sef.get())) return false;
                GottenVarPtr<SymbolicDouble> t2 = term2.vg->getSymbolicDouble(sef.get());

                if (op == MINUS) result->minusSymbolicDouble(*t2, vs->getFullName() == term1.vg->getFullName());
                else if (op == MOD) result->modSymbolicDouble(*t2);
                else if (op == DIV) result->divSymbolicDouble(*t2);
                else throw runtime_error("Strange op encountered");

                vs->setSymbolicVariable(sef.get(), result.get());
                return true;
            }
            else //var and const
            {
                if (op == MINUS) result->addConst(-term2.d);
                else if (op == MOD) result->modConst(term2.d);
                else if (op == DIV)
                {
                    if (term2.d == 0)
                    {
                        sef->error(Reporter::ZERODIVISION, "", getLineNum());
                        return false;
                    }
                    else result->divConst(term2.d);
                }
                else throw runtime_error("Strange op encountered");

                vs->setSymbolicVariable(sef.get(), result.get());
                return true;
            }
        }
        else //const and var
        {
            if (!term2.vg->check(sef.get())) return false;
            GottenVarPtr<SymbolicDouble> rhs = term2.vg->getSymbolicDouble(sef.get());
            GottenVarPtr<SymbolicDouble> result = vs->getSymbolicDouble(sef.get());

            if (op == MINUS)
            {
                result->clipTUpperBound(term1.d - rhs->getTLowerBound());
                result->clipTLowerBound(term2.d - rhs->getTUpperBound());
            }
            else if (op == MOD)
            {
                double lb, ub;
                lb = rhs->getTLowerBound();
                ub = rhs->getTUpperBound();
                if (lb == ub)
                {
                    if (lb == 0)
                    {
                        sef->error(Reporter::ZERODIVISION, "RHS must be zero", getLineNum());
                        return false;
                    }
                    result->setTConstValue(fmod(term1.d, lb));
                }
                else
                {
                    if (lb <= 0 && ub >= 0) sef->warn(Reporter::ZERODIVISION, "RHS could be zero", getLineNum());
                    result->setTLowerBound(0);
                    result->clipTUpperBound(max(abs(lb), abs(ub)));
                }
            }
            else if (op == DIV)
            {
                //term1.d should be positive
                double lb, ub;
                lb = rhs->getTLowerBound();
                ub = rhs->getTUpperBound();
                if (lb == ub)
                {
                    if (lb == 0)
                    {
                        sef->error(Reporter::ZERODIVISION, "RHS must be zero", getLineNum());
                        return false;
                    }
                    result->setTConstValue(term1.d / lb);
                }
                else
                {
                    if (lb <= 0 && ub >= 0) sef->warn(Reporter::ZERODIVISION, "RHS could be zero", getLineNum());
                    if (lb <= 0)
                    {
                        if (ub <= 0)
                        {
                            if (lb == 0) result->removeLowerBound();
                            else result->setTUpperBound(term2.d / lb);
                            if (ub == 0) result->removeLowerBound();
                            else result->setTLowerBound(term2.d / ub);
                        }
                        else
                        {
                            if (ub == 0) result->removeUpperBound();
                            else result->setTUpperBound(term2.d / ub);
                            if (lb == 0) result->removeLowerBound();
                            result->setTLowerBound(term2.d / lb);
                        }
                    }
                    else
                    {
                        result->setTLowerBound(term2.d / ub);
                        result->setTUpperBound(term2.d / lb);
                    }
                }
            }
            else throw runtime_error("Strange op encountered");

            vs->setSymbolicVariable(sef.get(), result.get());
        }
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

