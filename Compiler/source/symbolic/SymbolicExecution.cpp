#include "SymbolicExecution.h"
#include "../Command.h"

using namespace std;
using namespace SymbolicExecution;

/*SymbolicExecutionFringe*/
SymbolicExecutionFringe::SymbolicExecutionFringe(Reporter &r) :
        reporter(r),
        parent{},
        currentStack(make_shared<SymbolicStack>()),
        symbolicVarSet(make_shared<SymbolicVarSet>()){}

SymbolicExecutionFringe::SymbolicExecutionFringe(std::shared_ptr<SymbolicExecutionFringe> p):
        parent(p),
        reporter(p->reporter),
        currentStack(make_shared<SymbolicStack>(p->currentStack)),
        symbolicVarSet(make_shared<SymbolicVarSet>(p->symbolicVarSet)) {}


void SymbolicExecutionFringe::error(Reporter::AlertType a, string s, int linenum)
{
    if (linenum != -1) s += " (line " + to_string(linenum) + ")";
    reporter.error(a, s);
    feasable = false;
}

void SymbolicExecutionFringe::warn(Reporter::AlertType a, string s, int linenum)
{
    if (linenum != -1) s += " (line " + to_string(linenum) + ")";
    reporter.warn(a, s);
}

bool SymbolicExecutionFringe::hasSeen(string state)
{
    return pathConditions.find(state) != pathConditions.end();
}

bool SymbolicExecutionFringe::isFeasable()
{
    if (!symbolicVarSet->isFeasable()) feasable = false;
    return feasable;
}

/*SymbolicExecutionManager*/
void SymbolicExecutionManager::search()
{
    shared_ptr<SymbolicExecutionFringe> sef = make_shared<SymbolicExecutionFringe>(reporter);
    if (!visitNode(sef, cfg.getFirst()))
    {
        reporter.warn(Reporter::GENERIC, "No feasable path was found through the program");
    }
    for (auto& p : feasableVisits)
    {
        if (p.second == 0) // no feasable visits - remove
        {
            shared_ptr<CFGNode> lonelyNode = cfg.getNode(p.first);
            if (lonelyNode->getCompSuccess() != nullptr) lonelyNode->getCompSuccess()->removeParent(lonelyNode);
            if (lonelyNode->getCompFail() != nullptr) lonelyNode->getCompFail()->removeParent(lonelyNode);
            for (auto& parentPair : lonelyNode->getPredecessors())
            {
                shared_ptr<CFGNode> parent = parentPair.second;
                if (parent->getCompSuccess() != nullptr &&
                        parent->getCompSuccess()->getName() == lonelyNode->getName()) parent->setCompSuccess(nullptr);

                else if (parent->getCompFail() != nullptr && parent->getCompFail()->getName() == lonelyNode->getName())
                {
                    parent->setCompFail(parent->getCompSuccess());
                    parent->setComp(nullptr);
                    parent->setCompSuccess(nullptr);
                }
                else throw runtime_error("bad parent");
                parent->setComp(nullptr);
            }
            cfg.removeNode(p.first);
        }
    }
}

std::shared_ptr<CFGNode>
SymbolicExecutionManager::getFailNode(std::shared_ptr<SymbolicExecutionFringe> returningSEF, std::shared_ptr<CFGNode> n)
{
    shared_ptr<CFGNode> failNode = n->getCompFail();
    if (failNode == nullptr) //return to top of stack
    {
        if (returningSEF->currentStack->getTopType() != SymbolicStackMemberType::STATE)
        {
            returningSEF->error(Reporter::BAD_STACK_USE, "Tried to jump to a non state", n->getJumpline()); //probably my fault
            return nullptr;
        }
        failNode = n->getParent().getNode(returningSEF->currentStack->popState(), false);
        if (failNode == nullptr)
        {
            returningSEF->error(Reporter::BAD_STACK_USE, "Tried to jump to a nonexisting state", n->getJumpline());
            return nullptr;
        }
    }
    failNode->addParent(n);
    return failNode;
}

bool SymbolicExecutionManager::visitNode(shared_ptr<SymbolicExecutionFringe> sef, shared_ptr<CFGNode> n)
{
    if (!sef->isFeasable()) return false;
    else if (sef->hasSeen(n->getName())) return true;

    for (shared_ptr<AbstractCommand> command : n->getInstrs())
    {
        if (!command->acceptSymbolicExecution(sef)) return false;
    }

    feasableVisits[n->getName()]++;

    //no templated lambdas yet
    auto branchOnType =
    [&n, this] (shared_ptr<SymbolicExecutionFringe> sef, string lhs, Relations::Relop op, string rhs, VariableType t,
          bool rev = false)
    {
        if (t == STRING) return branch<string>(sef, n, lhs, op, rhs, rev);
        else if (t == DOUBLE) return branch<double>(sef, n, lhs, op, stod(rhs), rev);
        else throw runtime_error("bad vtype");
    };

    shared_ptr<JumpOnComparisonCommand> jocc = n->getComp();
    if (jocc != nullptr) //is a conditional jump
    {
        //check for const comparison
        if (jocc->term1Type != AbstractCommand::StringType::ID
            && jocc->term2Type != AbstractCommand::StringType::ID)
        {
            if (jocc->term1Type != jocc->term2Type)
            {
                sef->error(Reporter::TYPE, "Tried to compare literals of different types", jocc->getLineNum());
                return false;
            }

            else
            {
                string trans = jocc->translation();
                sef->reporter.optimising(Reporter::USELESS_OP, "Constant comparison: '" + trans.substr(0, trans.length() - 2) + "'");

                //replace conditionals with true/false
                bool isTrue;
                if (jocc->term1Type == AbstractCommand::StringType::DOUBLELIT)
                {
                    double d1 = stod(jocc->term1);
                    double d2 = stod(jocc->term2);
                    isTrue = Relations::evaluateRelop<double>(d1, jocc->op, d2);
                }
                else isTrue = Relations::evaluateRelop<string>(jocc->term1, jocc->op, jocc->term2);

                if (isTrue)
                {
                    if (n->getCompFail() != nullptr) n->getCompFail()->removeParent(n);
                    n->setCompFail(n->getCompSuccess());
                }
                else n->getCompSuccess()->removeParent(n);
                n->getComp().reset();
                n->setComp(nullptr);
                n->getCompSuccess().reset();
                n->setCompSuccess(nullptr);

                shared_ptr<CFGNode> nextNode = getFailNode(sef, n);
                if (nextNode == nullptr) return false;
                return visitNode(sef, nextNode);
            }
        }
        else //actually have to do some work
        {
            //note: JOCC constructor ensures that if there is a var there is a var on the LHS
            VarPointer LHS = sef->symbolicVarSet->findVar(jocc->term1);
            if (LHS == nullptr)
            {
                sef->error(Reporter::UNDECLARED_USE, "'" + jocc->term1 + "' used without being declared",
                           jocc->getLineNum());
                return false;
            }
            if (!LHS->isDefined()) sef->warn(Reporter::UNINITIALISED_USE,
                                             "'" + LHS->getName() + "' used before being defined", jocc->getLineNum());

            //check if we can meet the comparison - search if so
            if (jocc->term2Type != AbstractCommand::StringType::ID) //comparing to a literal
            {
                if ((jocc->term2Type == AbstractCommand::StringType::DOUBLELIT && LHS->getType() != DOUBLE)
                        || (jocc->term2Type == AbstractCommand::StringType::STRINGLIT && LHS->getType() != STRING))
                {
                    sef->error(Reporter::TYPE, "'" + jocc->term1 + "' (type " + TypeEnumNames[LHS->getType()]
                                               + ")  compared to a different type",
                               jocc->getLineNum());
                    return false;
                }

                string& rhs = jocc->term2;

                if (LHS->isDetermined())
                {
                    if (LHS->meetsConstComparison(jocc->op, rhs))
                    {
                        sef->pathConditions[n->getName()] = make_shared<Condition>(LHS->getName(), jocc->op, rhs);
                        return visitNode(sef, n->getCompSuccess());
                    }
                }

                switch (LHS->canMeet(jocc->op, rhs))
                {
                    case SymbolicVariable::CANT:
                    {
                        sef->pathConditions[n->getName()] = make_shared<Condition>(LHS->getName(),
                                                                                   Relations::negateRelop(jocc->op), rhs);

                        shared_ptr<CFGNode> nextNode = getFailNode(sef, n);
                        if (nextNode == nullptr) return false;
                        return visitNode(sef, nextNode);
                    }
                    case SymbolicVariable::MAY:
                    {
                        return branchOnType(sef, LHS->getName(), jocc->op,
                                                           jocc->term2, LHS->getType(), false);
                    }
                    case SymbolicVariable::MUST:
                    {
                        sef->pathConditions[n->getName()] = make_shared<Condition>(LHS->getName(), jocc->op, rhs);

                        shared_ptr<CFGNode> nextNode = getFailNode(sef, n);
                        if (nextNode == nullptr) return false;
                        return visitNode(sef, nextNode);
                    }
                    default:
                        throw runtime_error("very weird enum");
                }
            }
            else //comparing to another variable
            {
                VarPointer RHS = sef->symbolicVarSet->findVar(jocc->term2);
                if (RHS == nullptr)
                {
                    sef->error(Reporter::UNDECLARED_USE, "'" + jocc->term2 + "' used without being declared",
                               jocc->getLineNum());
                    return false;
                }

                if (LHS->getType() != RHS->getType())
                {
                    sef->error(Reporter::TYPE, "'" + jocc->term1 + "' (type " + TypeEnumNames[LHS->getType()] +
                            ") compared to '" + jocc->term2 + "' (type " + TypeEnumNames[RHS->getType()] + ")",
                               jocc->getLineNum());
                    return false;
                }

                if (LHS->isDetermined())
                {
                    if (RHS->isDetermined())
                    {
                        if (LHS->meetsConstComparison(jocc->op, RHS->getConstString()))
                        {
                            sef->pathConditions[n->getName()] = make_shared<Condition>(LHS->getName(), jocc->op,
                                                                                       RHS->getName() + "(= " +  RHS->getConstString() + ")");
                            return visitNode(sef, n->getCompSuccess());
                        }
                        else
                        {
                            shared_ptr<CFGNode> nextnode = getFailNode(sef, n);
                            if (nextnode == nullptr) return false;
                            sef->pathConditions[n->getName()] = make_shared<Condition>(LHS->getName(), Relations::negateRelop(jocc->op),
                                                                                       RHS->getName() + "(= " +  RHS->getConstString() + ")");
                            return visitNode(sef, n);
                        }

                    }
                    else //rhs undetermined, lhs determined
                    {
                        Relations::Relop mirroredOp = Relations::mirrorRelop(jocc->op);
                        return branchOnType(sef, RHS->getName(), mirroredOp,
                                                           LHS->getConstString(), RHS->getType(), true);
                    }
                }
                else if (RHS->isDetermined()) return branchOnType(sef, LHS->getName(), jocc->op,
                                                                                 RHS->getConstString(), LHS->getType(), false);

                //neither are determined here
                if (LHS->getType() == STRING)
                {
                    VarTemplatePointer<string> LHS = static_pointer_cast<SymbolicVariableTemplate<string>>(LHS);
                    VarTemplatePointer<string> RHS = static_pointer_cast<SymbolicVariableTemplate<string>>(RHS);
                    if (varBranch<string>(sef, n, LHS, jocc->op, RHS)) return true;
                    return false;
                }
                else if (LHS->getType() == DOUBLE)
                {
                    VarTemplatePointer<double> LHS = static_pointer_cast<SymbolicVariableTemplate<double>>(LHS);
                    VarTemplatePointer<double> RHS = static_pointer_cast<SymbolicVariableTemplate<double>>(RHS);
                    if (varBranch<double>(sef, n, LHS, jocc->op, RHS)) return true;
                    return false;
                }
            }
        }
    }
    else //unconditional jump
    {
        if (n->getName() == cfg.getLast()->getName()) return true;
        shared_ptr<CFGNode> retNode = getFailNode(sef, n);
        if (retNode == nullptr) return false;
        return visitNode(sef, retNode);
    }
}

//these things below will usually be called when we already have
//a ptr to the vars but we want to copy that var into the 'new scope'W
template <typename T>
bool SymbolicExecutionManager::branch(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n, std::string lhsvar,
                                      Relations::Relop op, const T& rhsconst, bool reverse)
{
    switch(op)
    {
        case Relations::EQ:
            return branchEQ<T>(sef, n, lhsvar, rhsconst, reverse);
        case Relations::NE:
            return branchNE<T>(sef, n, lhsvar, rhsconst, reverse);
        case Relations::LT:
            return branchLT<T>(sef, n, lhsvar, rhsconst, reverse);
        case Relations::LE:
            return branchLE<T>(sef, n, lhsvar, rhsconst, reverse);
        case Relations::GT:
            return branchGT<T>(sef, n, lhsvar, rhsconst, reverse);
        case Relations::GE:
            return branchGE<T>(sef, n, lhsvar, rhsconst, reverse);
        default:
            throw runtime_error("bad relop");
    }
}

template <typename T>
bool SymbolicExecutionManager::branchEQ(shared_ptr<SymbolicExecutionFringe> sef, shared_ptr<CFGNode> n,
                                        string lhsvar, const T& rhsconst, bool reverse)
{
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVarOfType<T>(lhsvar)->setUpperBound(rhsconst, false);
    seflt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::LT, rhsconst);

    bool feasable;
    if (reverse) feasable = visitNode(seflt, n->getCompSuccess());
    else
    {
        shared_ptr<CFGNode> failNode = getFailNode(seflt, n);
        if (failNode == nullptr) return false;
        feasable = visitNode(seflt, failNode);
    }
    seflt.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    sefeq->symbolicVarSet->findVarOfType<T>(lhsvar)->setConstValue(rhsconst);
    sefeq->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::EQ, rhsconst);
    if (reverse)
    {
        shared_ptr<CFGNode> failNode = getFailNode(sefeq, n);
        if (failNode == nullptr) return false;
        feasable = visitNode(sefeq, failNode);
    }
    else feasable = visitNode(sefeq, n->getCompSuccess());
    sefeq.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVarOfType<T>(lhsvar)->setLowerBound(rhsconst, true);
    sefgt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::GT, rhsconst);

    if (reverse) feasable = visitNode(sefgt, n->getCompSuccess());
    else
    {
        shared_ptr<CFGNode> failNode = getFailNode(sefgt, n);
        if (failNode == nullptr) return false;
        feasable = visitNode(sefgt, failNode);
    }

    sefgt.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::branchNE(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                        std::string lhsvar, const T& rhsconst, bool reverse)
{
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVarOfType<T>(lhsvar)->setUpperBound(rhsconst, false);
    seflt->pathConditions[n->getName()] = make_shared<Condition>(lhsvar, Relations::LT, rhsconst);
    bool feasable;
    if (reverse)
    {
        shared_ptr<CFGNode> failNode = getFailNode(seflt, n);
        if (failNode == nullptr) return false;
        feasable = visitNode(seflt, failNode);
    }
    else feasable = visitNode(seflt, n->getCompSuccess());
    seflt.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    sefeq->symbolicVarSet->findVarOfType<T>(lhsvar)->setConstValue(rhsconst);
    sefeq->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::EQ, rhsconst);
    if (reverse) feasable = visitNode(seflt, n->getCompSuccess());
    else
    {
        shared_ptr<CFGNode> failNode = getFailNode(sefeq, n);
        if (failNode == nullptr) return false;
        feasable = visitNode(sefeq, failNode);
    }
    sefeq.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVarOfType<T>(lhsvar)->setLowerBound(rhsconst, true);
    sefgt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::GT, rhsconst);
    if (reverse)
    {
        shared_ptr<CFGNode> failNode = getFailNode(seflt, n);
        if (failNode == nullptr) return false;
        feasable = visitNode(seflt, failNode);
    }
    else feasable = visitNode(seflt, n->getCompSuccess());
    sefgt.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::branchLT(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                        std::string lhsvar, const T& rhsconst, bool reverse)
{
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVarOfType<T>(lhsvar)->setUpperBound(rhsconst, false);
    seflt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::LT, rhsconst);

    bool feasable;
    if (reverse)
    {
        shared_ptr<CFGNode> failNode = getFailNode(seflt, n);
        if (failNode == nullptr) return false;
        feasable = visitNode(seflt, failNode);
    }
    else feasable = visitNode(seflt, n->getCompSuccess());
    seflt.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    sefge->symbolicVarSet->findVarOfType<T>(lhsvar)->setLowerBound(rhsconst);
    sefge->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::GE, rhsconst);
    if (reverse) feasable = visitNode(sefge, n->getCompSuccess());
    else
    {
        shared_ptr<CFGNode> failNode = getFailNode(sefge, n);
        if (failNode == nullptr) return false;
        feasable = visitNode(sefge, failNode);
    }
    sefge.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::branchLE(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                        std::string lhsvar, const T& rhsconst, bool reverse)
{
    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    sefle->symbolicVarSet->findVarOfType<T>(lhsvar)->setUpperBound(rhsconst);
    sefle->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::LE, rhsconst);
    bool feasable;
    if (reverse)
    {
        shared_ptr<CFGNode> failNode = getFailNode(sefle, n);
        if (failNode == nullptr) return false;
        feasable = visitNode(sefle, failNode);
    }
    else feasable = visitNode(sefle, n->getCompSuccess());
    sefle.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVarOfType<T>(lhsvar)->setLowerBound(rhsconst, true);
    sefgt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::GT, rhsconst);
    if (reverse) feasable = visitNode(sefgt, n->getCompSuccess());
    else
    {
        shared_ptr<CFGNode> failNode = getFailNode(sefgt, n);
        if (failNode == nullptr) return false;
        feasable = visitNode(sefgt, failNode);
    }
    sefgt.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::branchGT(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                        std::string lhsvar, const T& rhsconst, bool reverse)
{
    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVarOfType<T>(lhsvar)->setLowerBound(rhsconst, false);
    sefgt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::GT, rhsconst);
    bool feasable;
    if (reverse)
    {
        shared_ptr<CFGNode> failNode = getFailNode(sefgt, n);
        if (failNode == nullptr) return false;
        feasable = visitNode(sefgt, failNode);
    }
    else feasable = visitNode(sefgt, n->getCompSuccess());
    sefgt.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    sefle->symbolicVarSet->findVarOfType<T>(lhsvar)->setUpperBound(rhsconst);
    sefle->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::LE, rhsconst);

    if (reverse) feasable = visitNode(sefle, n->getCompSuccess());
    else
    {
        shared_ptr<CFGNode> failNode = getFailNode(sefle, n);
        if (failNode == nullptr) return false;
        feasable = visitNode(sefle, failNode);
    }
    sefle.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::branchGE(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                        std::string lhsvar, const T& rhsconst, bool reverse)
{
    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    sefge->symbolicVarSet->findVarOfType<T>(lhsvar)->setLowerBound(rhsconst);
    sefge->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::GE, rhsconst);

    bool feasable;
    if (reverse)
    {
        shared_ptr<CFGNode> failNode = getFailNode(sefge, n);
        if (failNode == nullptr) return false;
        feasable = visitNode(sefge, failNode);
    }
    else feasable = visitNode(sefge, n->getCompSuccess());
    sefge.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVarOfType<T>(lhsvar)->setUpperBound(rhsconst, true);
    seflt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::LT, rhsconst);
    if (reverse) feasable = visitNode(seflt, n->getCompSuccess());
    else
    {
        shared_ptr<CFGNode> failNode = getFailNode(seflt, n);
        if (failNode == nullptr) return false;
        feasable = visitNode(seflt, failNode);
    }
    seflt.reset();
    return feasable;
}

//branching on var comparison
template <typename T>
bool SymbolicExecutionManager::varBranch(shared_ptr<SymbolicExecutionFringe> sef, shared_ptr<CFGNode> n,
                                         VarTemplatePointer<T> LHS, Relations::Relop op,
                                         VarTemplatePointer<T> RHS)
{
    switch(op)
    {
        case Relations::EQ:
            return varBranchEQ<T>(sef, n, LHS, RHS);
        case Relations::NE:
            return varBranchNE<T>(sef, n, LHS, RHS);
        case Relations::LT:
            return varBranchLT<T>(sef, n, LHS, RHS);
        case Relations::LE:
            return varBranchLE<T>(sef, n, LHS, RHS);
        case Relations::GT:
            return varBranchGT<T>(sef, n, LHS, RHS);
        case Relations::GE:
            return varBranchGE<T>(sef, n, LHS, RHS);
        default:
            throw runtime_error("bad relop");
    }
}

template <typename T>
bool SymbolicExecutionManager::varBranchGE(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                           VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar)
{
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    shared_ptr<CFGNode> failNode = getFailNode(seflt, n);
    if (failNode == nullptr) return false;
    seflt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar->getName(), Relations::LT, rhsvar->getName());
    if (seflt->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipUpperBound(rhsvar->getUpperBound(), false)) return false;
    bool feasable = visitNode(seflt, failNode);
    seflt.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    if (!sefge->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipLowerBound(rhsvar->getLowerBound())) return false;
    sefge->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar->getName(), Relations::GE, rhsvar->getName());
    feasable = visitNode(sefge, n->getCompSuccess());
    sefge.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::varBranchGT(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                              VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar)
{
    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    shared_ptr<CFGNode> failNode = getFailNode(sefle, n);
    if (failNode == nullptr) return false;
    if (!sefle->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipUpperBound(rhsvar->getUpperBound())) return false;
    sefle->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar->getName(), Relations::LE, rhsvar->getName());
    bool feasable = visitNode(sefle, failNode);
    sefle.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar->getName(), Relations::GT, rhsvar->getName());
    if (!sefgt->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipLowerBound(rhsvar->getLowerBound(), false)) return false;
    feasable = visitNode(sefgt, n->getCompSuccess());
    sefgt.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::varBranchLT(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                              VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar)
{
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    shared_ptr<CFGNode> failNode = getFailNode(seflt, n);
    if (failNode == nullptr) return false;
    if (!seflt->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipUpperBound(rhsvar->getUpperBound(), false)) return false;
    seflt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar->getName(), Relations::LT, rhsvar->getName());
    bool feasable = visitNode(seflt, n->getCompSuccess());
    seflt.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    if (!sefge->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipLowerBound(rhsvar->getLowerBound())) return false;
    sefge->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar->getName(), Relations::GE, rhsvar->getName());
    feasable = visitNode(sefge, failNode);
    sefge.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::varBranchLE(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                              VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar)
{
    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    shared_ptr<CFGNode> failNode = getFailNode(sefle, n);
    if (failNode == nullptr) return false;
    if (!sefle->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipUpperBound(rhsvar->getUpperBound())) return false;
    sefle->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar->getName(), Relations::LE, rhsvar->getName());
    bool feasable = visitNode(sefle, n->getCompSuccess());
    sefle.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    if (!sefgt->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipLowerBound(rhsvar->getLowerBound())) return false;
    sefgt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar->getName(), Relations::GT, rhsvar->getName());
    feasable = visitNode(sefgt, n->getCompSuccess());
    sefgt.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::varBranchNE(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                           VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar)
{
    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    shared_ptr<CFGNode> failNode = getFailNode(sefeq, n);
    if (failNode == nullptr) return false;
    VarTemplatePointer<T>& glbvar = getGreatestLowerBound(lhsvar, rhsvar);
    if (glbvar->isBoundedBelow())
    {
        //pull the variables into sefeq
        VarTemplatePointer<T> lhsvar = sefeq->symbolicVarSet->findVarOfType<T>(lhsvar->getName());
        VarTemplatePointer<T> rhsvar = sefeq->symbolicVarSet->findVarOfType<T>(rhsvar->getName());

        const T& glb = glbvar->getLowerBound();
        if (!lhsvar->clipLowerBound(glb) || !rhsvar->clipLowerBound(glb)) return false;
    }
    VarTemplatePointer<T>& lubvar = getLeastUpperBound(lhsvar, rhsvar);
    if (lubvar->isBoundedAbove())
    {
        VarTemplatePointer<T> lhsvar = sefeq->symbolicVarSet->findVarOfType<T>(lhsvar->getName());
        VarTemplatePointer<T> rhsvar = sefeq->symbolicVarSet->findVarOfType<T>(rhsvar->getName());

        const T& lub = lubvar->getUpperBound();
        if (!lhsvar->clipUpperBound(lub) || !rhsvar->clipUpperBound(lub)) return false;
    }
    sefeq->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar->getName(), Relations::EQ, rhsvar->getName());
    bool feasable = visitNode(sefeq, failNode);
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    if (!sefgt->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipLowerBound(rhsvar->getUpperBound(), false)) return false;
    feasable = visitNode(sefgt, n->getCompSuccess());
    sefgt.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    if (!seflt->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipUpperBound(rhsvar->getLowerBound(), true)) return false;
    seflt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar->getName(), Relations::LT, rhsvar->getName());
    feasable = visitNode(seflt, n->getCompSuccess());
    seflt.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::varBranchEQ(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                           VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar)
{
    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    VarTemplatePointer<T>& glbvar = getGreatestLowerBound(lhsvar, rhsvar);
    if (glbvar->isBoundedBelow())
    {
        //pull the variables into sefeq
        VarTemplatePointer<T> lhsvar = sefeq->symbolicVarSet->findVarOfType<T>(lhsvar->getName());
        VarTemplatePointer<T> rhsvar = sefeq->symbolicVarSet->findVarOfType<T>(rhsvar->getName());

        const T& glb = glbvar->getLowerBound();
        if (!lhsvar->clipLowerBound(glb) || !rhsvar->clipLowerBound(glb)) return false;
    }
    VarTemplatePointer<T>& lubvar = getLeastUpperBound(lhsvar, rhsvar);
    if (lubvar->isBoundedAbove())
    {
        VarTemplatePointer<T> lhsvar = sefeq->symbolicVarSet->findVarOfType<T>(lhsvar->getName());
        VarTemplatePointer<T> rhsvar = sefeq->symbolicVarSet->findVarOfType<T>(rhsvar->getName());

        const T& lub = lubvar->getUpperBound();
        if (!lhsvar->clipUpperBound(lub) || !rhsvar->clipUpperBound(lub)) return false;
    }
    sefeq->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar->getName(), Relations::EQ, rhsvar->getName());
    bool feasable = visitNode(sefeq, n->getCompSuccess());
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    shared_ptr<CFGNode> failNode = getFailNode(sefgt, n);
    if (failNode == nullptr) return false;
    if (!sefgt->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipLowerBound(rhsvar->getUpperBound(), false)) return false;
    feasable = visitNode(sefgt, n->getCompSuccess());
    sefgt.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    if (!seflt->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipUpperBound(rhsvar->getLowerBound(), true)) return false;
    failNode = getFailNode(sef, n);
    if (failNode == nullptr) return false;
    seflt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar->getName(), Relations::LT, rhsvar->getName());
    feasable = visitNode(seflt, failNode);
    seflt.reset();
    return feasable;
}

template <typename T>
VarTemplatePointer<T>& SymbolicExecutionManager::getGreatestLowerBound(VarTemplatePointer<T>& lhsvar, VarTemplatePointer<T>& rhsvar)
{
    if (!lhsvar->isBoundedBelow()) return rhsvar;
    else if (!rhsvar->isBoundedBelow()) return lhsvar;
    else return (lhsvar->getLowerBound() > rhsvar->getLowerBound()) ? lhsvar : rhsvar;
}

template <typename T>
VarTemplatePointer<T>& SymbolicExecutionManager::getLeastUpperBound(VarTemplatePointer<T>& lhsvar, VarTemplatePointer<T>& rhsvar)
{
    if (!lhsvar->isBoundedAbove()) return rhsvar;
    else if (!rhsvar->isBoundedAbove()) return lhsvar;
    else return (lhsvar->getUpperBound() < rhsvar->getUpperBound()) ? lhsvar : rhsvar;
}
