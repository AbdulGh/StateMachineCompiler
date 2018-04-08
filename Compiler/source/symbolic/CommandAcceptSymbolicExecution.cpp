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
    if (pushesState()) sef->symbolicStack->pushState(s);

    else if (atom.isHolding())
    {
        if (atom.getVarWrapper()->check(sef.get()))
        {
            GottenVarPtr<SymbolicDouble> found = atom.getVarWrapper()->getSymbolicDouble(sef.get());

            if (!found->isFeasable()) throw std::runtime_error("should be feasable");
            else if (!found->isDefined())
            {
                sef->warn(Reporter::UNINITIALISED_USE, "'" + found->getName() + "' pushed without being defined", getLineNum());
            }

            sef->symbolicStack->pushVar(move(found));
        }
        else return false;
    }
    else sef->symbolicStack->pushDouble(atom.getLiteral());
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

    unique_ptr<SymbolicDouble> popped = sef->symbolicStack->popVar();
    if (vs->check(sef.get()))
    {
        vs->setSymbolicDouble(sef.get(), popped.get());
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
        GottenVarPtr<SymbolicDouble> svp = atom.getVarWrapper()->getSymbolicDouble(sef.get());
        if (!svp->isFeasable()) return false;
        svp->define();
        vs->setSymbolicDouble(sef.get(), svp.get());
    }
    else vs->getSymbolicDouble(sef.get())->setConstValue(atom.getLiteral());

    return true;
}

bool EvaluateExprCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat)
{
    if (!repeat)
    {
        int debug;
        debug = 2;
    }

    if (op == AND || op == OR) throw runtime_error("Bitwise operations not supported by verifier");
    else if (op == PLUS || op == MULT)
    {
        auto multConst = [&, this, sef, repeat] (SymbolicDouble* result, double c) -> void
        {
            double absc = abs(c);
            if (absc == 0) result->setConstValue(0);
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
                    if (result->getLowerBound() > 0) result->setLowerBound(0);
                    if (result->getUpperBound() < 0) result->setUpperBound(0);
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

        if (term1.isHolding() && term2.isHolding())
        {
            if (!term1.getVarWrapper()->check(sef.get()) || !term2.getVarWrapper()->check(sef.get())) return false;

            unique_ptr<SymbolicDouble> result = term1.getVarWrapper()->getSymbolicDouble(sef.get())->clone();
            GottenVarPtr<SymbolicDouble> t2 = term2.getVarWrapper()->getSymbolicDouble(sef.get());

            if (result->isDetermined())
            {
                if (op == PLUS) addConst(t2.get(), result->getConstValue());
                else multConst(t2.get(), result->getConstValue());
            }
            else if (t2->isDetermined())
            {
                if (op == PLUS) addConst(result.get(), t2->getConstValue());
                else multConst(result.get(), t2->getConstValue());
            }
            else
            {
                if (repeat)
                {
                    if (t2->getUpperBound() > 0) result->maxUpperBound();
                    if (t2->getLowerBound() < 0) result->minLowerBound();
                }
                else
                {
                    if (op == PLUS)
                    {
                        std::string vsName = vs->getFullName();
                        bool increment = vsName == term1.getVarWrapper()->getFullName() || vsName == term2.getVarWrapper()->getFullName();
                        result->addSymbolicDouble(*t2, increment);
                    }
                    else result->multSymbolicDouble(*t2);
                }
            }
            result->define();
            vs->setSymbolicDouble(sef.get(), result.get());
            return true;
        }
        else
        {
            double constTerm; VarWrapper* varWrapper;
            if (term1.isHolding())
            {
                if (!term1.getVarWrapper()->check(sef.get())) return false;
                varWrapper = term1.getVarWrapper();
                constTerm = term2.getLiteral();
            }
            else
            {
                if (!term2.getVarWrapper()->check(sef.get())) return false;
                varWrapper = term2.getVarWrapper();
                constTerm = term1.getLiteral();
            }

            unique_ptr<SymbolicDouble> result = varWrapper->getSymbolicDouble(sef.get())->clone();

            if (op == PLUS) addConst(result.get(), constTerm);
            else multConst(result.get(), constTerm);

            result->define();
            vs->setSymbolicDouble(sef.get(), result.get());
            return true;
        }
    }

    else //not commutative
    {
        auto varModVar = [&sef, this] (SymbolicDouble* result, SymbolicDouble* t2v) -> void
        {
            if (t2v->getLowerBound() <= 0 && t2v->getUpperBound() >= 0)
            {
                sef->warn(Reporter::ZERODIVISION, "RHS could potentially be 0", getLineNum());
            }

            result->clipLowerBound(0);
            result->clipUpperBound(max(abs(t2v->getLowerBound()), abs(t2v->getUpperBound())));
        };

        if (term1.isHolding())
        {
            unique_ptr<SymbolicDouble> result = term1.getVarWrapper()->getSymbolicDouble(sef.get())->clone();
            if (repeat)
            {
                double t2c;

                if (term2.isHolding())
                {
                    if (!term2.getVarWrapper()->check(sef.get())) return false;
                    GottenVarPtr<SymbolicDouble> t2v = term2.getVarWrapper()->getSymbolicDouble(sef.get());
                    if (t2v->isDetermined()) t2c = t2v->getConstValue();
                    else
                    {
                        if (op == MINUS)
                        {
                            if (t2v->getUpperBound() > 0) result->minLowerBound();
                            else if (t2v->getLowerBound() < 0) result->maxUpperBound();
                        }
                        else if (op == MOD) varModVar(result.get(), t2v.get());
                        else if (op == DIV) //todo this is wrong if the rhs can be negative
                        {
                            if (t2v->getLowerBound() <= 0 && t2v->getUpperBound() >= 0)
                            {
                                sef->warn(Reporter::ZERODIVISION, "RHS could potentially be 0", getLineNum());
                            }

                            result->minLowerBound();
                            result->maxUpperBound();

                            /*double abslb = abs(t2v->getLowerBound());
                            double absub = abs(t2v->getUpperBound());
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
                                if (result->getLowerBound() > 0)
                                {
                                    result->minLowerBound()
                                    result->setLowerBound(max(result->getRepeatLowerBound(), 0.0)); //todo next this
                                }

                                if (result->getUpperBound() < 0)
                                {
                                    result->setUpperBound(min(result->getTRepeatUpperBound(), 0.0));
                                }
                            }
                            if (maxabs > 1)
                            {
                                result->minLowerBound();
                                result->maxUpperBound();
                            }*/
                        }
                        else throw runtime_error("Strange op encountered");

                        result->define();
                        vs->setSymbolicDouble(sef.get(), result.get());
                        return true;
                    }
                }
                else t2c = term2.getLiteral();

                if (op == MINUS)
                {
                    if (t2c > 0) result->minLowerBound();
                    else if (t2c < 0) result->maxUpperBound();
                    result->addConst(-t2c);
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
                            double maxAbs = max(abs(result->getLowerBound()), abs(result->getUpperBound()));
                            result->setLowerBound(-maxAbs);
                            result->setUpperBound(maxAbs);
                        }
                        else
                        {
                            result->minLowerBound();
                            result->maxUpperBound();
                        }
                    }
                }
                else throw runtime_error("Strange op encountered");
                result->define();
                vs->setSymbolicDouble(sef.get(), result.get());
                return true;
            }

            if (!term1.getVarWrapper()->check(sef.get())) return false;
            if (term2.isHolding()) //var and var
            {
                if (!term2.getVarWrapper()->check(sef.get())) return false;
                GottenVarPtr<SymbolicDouble> t2 = term2.getVarWrapper()->getSymbolicDouble(sef.get());

                if (op == MINUS) result->minusSymbolicDouble(*t2, vs->getFullName() == term1.getVarWrapper()->getFullName());
                else if (op == MOD) result->modSymbolicDouble(*t2);
                else if (op == DIV) result->divSymbolicDouble(*t2);
                else throw runtime_error("Strange op encountered");

                result->define();
                vs->setSymbolicDouble(sef.get(), result.get());
                return true;
            }
            else //var and const
            {
                if (op == MINUS) result->addConst(-term2.getLiteral());
                else if (op == MOD) result->modConst(term2.getLiteral());
                else if (op == DIV)
                {
                    if (term2.getLiteral() == 0)
                    {
                        sef->error(Reporter::ZERODIVISION, "", getLineNum());
                        return false;
                    }
                    else result->divConst(term2.getLiteral());
                }
                else throw runtime_error("Strange op encountered");
                result->define();

                vs->setSymbolicDouble(sef.get(), result.get());
                return true;
            }
        }
        else //const and var
        {
            if (!term2.getVarWrapper()->check(sef.get())) return false;
            GottenVarPtr<SymbolicDouble> rhs = term2.getVarWrapper()->getSymbolicDouble(sef.get());
            GottenVarPtr<SymbolicDouble> result = vs->getSymbolicDouble(sef.get());

            if (op == MINUS)
            {
                result->clipUpperBound(term1.getLiteral() - rhs->getLowerBound());
                result->clipLowerBound(term2.getLiteral() - rhs->getUpperBound());
            }
            else if (op == MOD)
            {
                double lb, ub;
                lb = rhs->getLowerBound();
                ub = rhs->getUpperBound();
                if (lb == ub)
                {
                    if (lb == 0)
                    {
                        sef->error(Reporter::ZERODIVISION, "RHS must be zero", getLineNum());
                        return false;
                    }
                    result->setConstValue(fmod(term1.getLiteral(), lb));
                }
                else
                {
                    if (lb <= 0 && ub >= 0) sef->warn(Reporter::ZERODIVISION, "RHS could be zero", getLineNum());
                    result->setLowerBound(0);
                    result->clipUpperBound(max(abs(lb), abs(ub)));
                }
            }
            else if (op == DIV)
            {
                //term1.getLiteral() should be positive
                double lb, ub;
                lb = rhs->getLowerBound();
                ub = rhs->getUpperBound();
                if (lb == ub)
                {
                    if (lb == 0)
                    {
                        sef->error(Reporter::ZERODIVISION, "RHS must be zero", getLineNum());
                        return false;
                    }
                    result->setConstValue(term1.getLiteral() / lb);
                }
                else
                {
                    if (lb <= 0 && ub >= 0) sef->warn(Reporter::ZERODIVISION, "RHS could be zero", getLineNum());
                    if (lb <= 0)
                    {
                        if (ub <= 0)
                        {
                            if (lb == 0) result->removeLowerBound();
                            else result->setUpperBound(term2.getLiteral() / lb);
                            if (ub == 0) result->removeLowerBound();
                            else result->setLowerBound(term2.getLiteral() / ub);
                        }
                        else
                        {
                            if (ub == 0) result->removeUpperBound();
                            else result->setUpperBound(term2.getLiteral() / ub);
                            if (lb == 0) result->removeLowerBound();
                            result->setLowerBound(term2.getLiteral() / lb);
                        }
                    }
                    else
                    {
                        result->setLowerBound(term2.getLiteral() / ub);
                        result->setUpperBound(term2.getLiteral() / lb);
                    }
                }
            }
            else throw runtime_error("Strange op encountered");
            result->define();

            vs->setSymbolicDouble(sef.get(), result.get());
            return true;
        }
    }
}

bool DeclareVarCommand::acceptSymbolicExecution(shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat)
{
    sef->symbolicVarSet->addVar(make_unique<SymbolicDouble>(name, sef->reporter));
    return true;
}

bool DeclareArrayCommand::acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef,
                                                  bool repeat)
{
    sef->symbolicVarSet->addArray(name, make_unique<SymbolicArray>(name, size, sef->reporter));
    return true;
}

bool NondetCommand::acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat)
{
    if (holding) getVarWrapper()->nondet(sef.get());
    else
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(s);
        if (!sa) throw runtime_error("Unknown array '" + s + "'");
        sa->nondet();
    }
    return true;
}