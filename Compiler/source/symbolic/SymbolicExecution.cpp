#include "SymbolicExecution.h"

using namespace std;
using namespace SymbolicExecution;

/*SymbolicExecutionFringe*/
SymbolicExecutionFringe::SymbolicExecutionFringe(Reporter &r, SymbolicExecutionFringe* p) :
        reporter(r),
        parent(p),
        currentStack(parent->currentStack),
        symbolicVarSet(parent->symbolicVarSet){}


void SymbolicExecutionFringe::error(Reporter::AlertType a, std::string s, int linenum)
{
    if (linenum != -1) s += " (line " + std::to_string(linenum) + ")";
    reporter.error(a, s);
    feasable = false;
}

void SymbolicExecutionFringe::warn(Reporter::AlertType a, std::string s, int linenum)
{
    if (linenum != -1) s += " (line " + std::to_string(linenum) + ")";
    reporter.warn(a, s);
}

bool SymbolicExecutionFringe::hasSeen(std::string state)
{
    return pathConditions.find(state) != pathConditions.end();
}

bool SymbolicExecutionFringe::isFeasable()
{
    if (!symbolicVarSet.isFeasable()) feasable = false;
    return feasable;
}

/*SymbolicExecutionManager*/
void SymbolicExecutionManager::search()
{
    SymbolicExecutionFringe* sef = new SymbolicExecutionFringe(reporter);
    if (!visitNode(sef, cfg.getFirst()))
    {
        reporter.warn(Reporter::GENERIC, "No feasable path was found through the program");
    }
}

bool SymbolicExecutionManager::visitNode(SymbolicExecutionFringe* sef, shared_ptr<CFGNode> n)
{
    if (!sef->isFeasable()) return false;
    else if (sef->hasSeen(n->getName())) return true;

    for (shared_ptr<AbstractCommand> command : n->getInstrs())
    {
        if (!command->acceptSymbolicExecution(sef)) return false;
    }

    feasableVisits[n->getName()]++;
    if (n->getName() == cfg.getLast()->getName()) return true;

    auto getFailNode = [&n] (SymbolicExecutionFringe* returningSEF) -> shared_ptr<CFGNode>
    {
        shared_ptr<CFGNode> failNode = n->getCompFail();
        if (failNode == nullptr) //return to top of stack
        {
            if (returningSEF->currentStack.getTopType() != SymbolicStackMemberType::STATE)
            {
                returningSEF->error(Reporter::BAD_STACK_USE, "Tried to jump to a non state", n->getJumpline()); //probably my fault
                delete returningSEF;
                return nullptr;
            }
            failNode = n->getParent().getNode(returningSEF->currentStack.popState(), false);
            if (failNode == nullptr)
            {
                returningSEF->error(Reporter::BAD_STACK_USE, "Tried to jump to a nonexisting state", n->getJumpline());
                delete returningSEF;
                return nullptr;
            }
        }
        return failNode;
    };

    shared_ptr<JumpOnComparisonCommand> jocc = n->getComp();
    if (jocc != nullptr) //is a conditional jump
    {
        //check for const comparison
        if (jocc->term1Type != JumpOnComparisonCommand::ComparitorType::ID
            && jocc->term2Type != JumpOnComparisonCommand::ComparitorType::ID)
        {
            if (jocc->term1Type != jocc->term2Type)
            {
                sef->error(Reporter::TYPE, "Tried to compare literals of different types", jocc->getLineNum());
                return false;
            }

            else
            {
                sef->reporter.optimising(Reporter::USELESS_OP, "Constant comparison: '" + jocc->translation() + "'");

                //replace conditionals with true/false
                bool isTrue;
                if (jocc->term1Type == JumpOnComparisonCommand::ComparitorType::DOUBLELIT)
                {
                    double d1 = stod(jocc->term1);
                    double d2 = stod(jocc->term2);
                    isTrue = Relations::evaluateRelop<double>(d1, jocc->op, d2);
                }
                else isTrue = Relations::evaluateRelop<string>(jocc->term1, jocc->op, jocc->term2);

                if (isTrue)
                {
                    n->getCompFail()->removeParent(n);
                    n->setCompFail(n->getCompSuccess());
                }
                else n->getCompSuccess()->removeParent(n);
                n->getComp().reset();
                n->setComp(nullptr);
            }
        }
        else //actually have to do some work
        {
            Relations::Relop op = jocc->op;

            //note: JOCC constructor ensures that if there is a var there is a var on the LHS
            VarPointer LHS = sef->symbolicVarSet.findVar(jocc->term1);
            if (LHS == nullptr)
            {
                sef->error(Reporter::UNDECLARED_USE, "'" + jocc->term1 + "' used without being declared",
                           jocc->getLineNum());
                return false;
            }

            //check if we can meet the comparison - search if so
            if (jocc->term2Type == JumpOnComparisonCommand::ComparitorType::DOUBLELIT)
            {
                if (LHS->getType() != DOUBLE)
                {
                    sef->error(Reporter::TYPE, "'" + jocc->term1 + "' (type " + TypeEnumNames[LHS->getType()]
                                               + ")  compared to a double literal",
                               jocc->getLineNum());
                    return false;
                }

                shared_ptr<SymbolicVariableTemplate<double>> LHS =
                        static_pointer_cast<SymbolicVariableTemplate<double>>(LHS);
                double rhs = stod(jocc->term2);

                switch (LHS->canMeet(jocc->op, rhs))
                {
                case SymbolicVariable::CANT:
                {
                    sef->pathConditions[n->getName()] = make_shared<Condition>(LHS->getName(),
                                                                               Relations::negateRelop(jocc->op), rhs);

                    shared_ptr<CFGNode> nextNode = getFailNode(sef);
                    if (nextNode == nullptr) return false;
                    return visitNode(sef, nextNode);
                }

                case SymbolicVariable::MAY:
                {
                    switch(jocc->op)
                    {
                        case Relations::EQ: //three new SEFs - less than, eq, greater
                        {
                            SymbolicExecutionFringe* seflt = new SymbolicExecutionFringe(sef->reporter, sef);
                            seflt->symbolicVarSet.findVar(LHS->getName())->setUpperBound(jocc->term2, false);
                            seflt->pathConditions[n->getName()]
                                    = make_shared<Condition>(LHS->getName(), Relations::LT, jocc->term2);
                            
                            shared_ptr<CFGNode> failNode = getFailNode(seflt);
                            if (failNode == nullptr) return false;
                            bool feasable = visitNode(seflt, failNode);
                            delete seflt;
                            if (!feasable) return false;

                            SymbolicExecutionFringe* sefeq = new SymbolicExecutionFringe(sef->reporter, sef);
                            sefeq->symbolicVarSet.findVar(LHS->getName())->setConstValue(jocc->term2);
                            sefeq->pathConditions[n->getName()]
                                    = make_shared<Condition>(LHS->getName(), Relations::EQ, jocc->term2);
                            feasable = visitNode(sefeq, n->getCompSuccess());
                            delete sefeq;
                            if (!feasable) return false;

                            SymbolicExecutionFringe* sefgt = new SymbolicExecutionFringe(sef->reporter, sef);
                            sefgt->symbolicVarSet.findVar(LHS->getName())->setLowerBound(jocc->term2, true);
                            sefgt->pathConditions[n->getName()]
                                    = make_shared<Condition>(LHS->getName(), Relations::GT, jocc->term2);
                            failNode = getFailNode(sefgt);
                            if (failNode == nullptr) return false;
                            feasable = visitNode(sefgt, failNode);
                            delete sefgt;
                            if (!feasable) return false;
                            break;
                        }
                        case Relations::NE: //ditto, just reverse the success/fail nodes
                        {
                            SymbolicExecutionFringe* seflt = new SymbolicExecutionFringe(sef->reporter, sef);
                            seflt->symbolicVarSet.findVar(LHS->getName())->setUpperBound(jocc->term2, false);
                            seflt->pathConditions[n->getName()]
                                    = make_shared<Condition>(LHS->getName(), Relations::LT, jocc->term2);

                            bool feasable = visitNode(seflt, n->getCompSuccess());
                            delete seflt;
                            if (!feasable) return false;

                            SymbolicExecutionFringe* sefeq = new SymbolicExecutionFringe(sef->reporter, sef);
                            sefeq->symbolicVarSet.findVar(LHS->getName())->setConstValue(jocc->term2);
                            sefeq->pathConditions[n->getName()]
                                    = make_shared<Condition>(LHS->getName(), Relations::EQ, jocc->term2);
                            shared_ptr<CFGNode> failNode = getFailNode(sefeq);
                            if (failNode == nullptr) return false;
                            feasable = visitNode(sefeq, n->getCompSuccess());
                            delete sefeq;
                            if (!feasable) return false;

                            SymbolicExecutionFringe* sefgt = new SymbolicExecutionFringe(sef->reporter, sef);
                            sefgt->symbolicVarSet.findVar(LHS->getName())->setLowerBound(jocc->term2, true);
                            sefgt->pathConditions[n->getName()]
                                    = make_shared<Condition>(LHS->getName(), Relations::GT, jocc->term2);
                            feasable = visitNode(sefgt, n->getCompSuccess());
                            delete sefgt;
                            if (!feasable) return false;
                            break;
                        }
                        case Relations::LT:
                        {
                            SymbolicExecutionFringe* seflt = new SymbolicExecutionFringe(sef->reporter, sef);
                            seflt->symbolicVarSet.findVar(LHS->getName())->setUpperBound(jocc->term2, false);
                            seflt->pathConditions[n->getName()]
                                    = make_shared<Condition>(LHS->getName(), Relations::LT, jocc->term2);

                            bool feasable = visitNode(seflt, n->getCompSuccess());
                            delete seflt;
                            if (!feasable) return false;

                            SymbolicExecutionFringe* sefge = new SymbolicExecutionFringe(sef->reporter, sef);
                            sefge->symbolicVarSet.findVar(LHS->getName())->setLowerBound(jocc->term2);
                            sefge->pathConditions[n->getName()]
                                    = make_shared<Condition>(LHS->getName(), Relations::GE, jocc->term2);
                            shared_ptr<CFGNode> failNode = getFailNode(seflt);
                            if (failNode == nullptr) return false;
                            feasable = visitNode(sefge, failNode);
                            delete sefge;
                            if (!feasable) return false;
                        }
                        case Relations::LE:
                        {
                            SymbolicExecutionFringe* seflt = new SymbolicExecutionFringe(sef->reporter, sef);
                            seflt->symbolicVarSet.findVar(LHS->getName())->setUpperBound(jocc->term2);
                            seflt->pathConditions[n->getName()]
                                    = make_shared<Condition>(LHS->getName(), Relations::LE, jocc->term2);

                            bool feasable = visitNode(seflt, n->getCompSuccess());
                            delete seflt;
                            if (!feasable) return false;

                            SymbolicExecutionFringe* sefge = new SymbolicExecutionFringe(sef->reporter, sef);
                            sefge->symbolicVarSet.findVar(LHS->getName())->setLowerBound(jocc->term2, true);
                            sefge->pathConditions[n->getName()]
                                    = make_shared<Condition>(LHS->getName(), Relations::GT, jocc->term2);
                            shared_ptr<CFGNode> failNode = getFailNode(seflt);
                            if (failNode == nullptr) return false;
                            feasable = visitNode(sefge, failNode);
                            delete sefge;
                            if (!feasable) return false;
                        }
                        case Relations::GT:
                        {
                            SymbolicExecutionFringe* seflt = new SymbolicExecutionFringe(sef->reporter, sef);
                            seflt->symbolicVarSet.findVar(LHS->getName())->setLowerBound(jocc->term2, false);
                            seflt->pathConditions[n->getName()]
                                    = make_shared<Condition>(LHS->getName(), Relations::GT, jocc->term2);

                            bool feasable = visitNode(seflt, n->getCompSuccess());
                            delete seflt;
                            if (!feasable) return false;

                            SymbolicExecutionFringe* sefge = new SymbolicExecutionFringe(sef->reporter, sef);
                            sefge->symbolicVarSet.findVar(LHS->getName())->setUpperBound(jocc->term2);
                            sefge->pathConditions[n->getName()]
                                    = make_shared<Condition>(LHS->getName(), Relations::LE, jocc->term2);
                            shared_ptr<CFGNode> failNode = getFailNode(seflt);
                            if (failNode == nullptr) return false;
                            feasable = visitNode(sefge, failNode);
                            delete sefge;
                            if (!feasable) return false;
                        }
                        case Relations::GE:
                        {
                            SymbolicExecutionFringe* seflt = new SymbolicExecutionFringe(sef->reporter, sef);
                            seflt->symbolicVarSet.findVar(LHS->getName())->setLowerBound(jocc->term2);
                            seflt->pathConditions[n->getName()]
                                    = make_shared<Condition>(LHS->getName(), Relations::GE, jocc->term2);

                            bool feasable = visitNode(seflt, n->getCompSuccess());
                            delete seflt;
                            if (!feasable) return false;

                            SymbolicExecutionFringe* sefge = new SymbolicExecutionFringe(sef->reporter, sef);
                            sefge->symbolicVarSet.findVar(LHS->getName())->setUpperBound(jocc->term2, true);
                            sefge->pathConditions[n->getName()]
                                    = make_shared<Condition>(LHS->getName(), Relations::LT, jocc->term2);
                            shared_ptr<CFGNode> failNode = getFailNode(seflt);
                            if (failNode == nullptr) return false;
                            feasable = visitNode(sefge, failNode);
                            delete sefge;
                            if (!feasable) return false;
                        }
                        default:
                            throw runtime_error("bad relop");
                    }
                }
                case SymbolicVariable::MUST:
                {
                    sef->pathConditions[n->getName()] = make_shared<Condition>(LHS->getName(), jocc->op, rhs);

                    shared_ptr<CFGNode> nextNode = getFailNode(sef);
                    if (nextNode == nullptr) return false;
                    return visitNode(sef, nextNode);
                }
                default:
                    throw runtime_error("very weird enum");
                }
            }
        }
    }
    else
    {
        shared_ptr<CFGNode> retNode = getFailNode(sef);
        if (retNode == nullptr) return false;
        return visitNode(sef, retNode);
    }
}