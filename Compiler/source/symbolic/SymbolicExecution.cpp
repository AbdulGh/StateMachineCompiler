#include "SymbolicExecution.h"

using namespace std;
using namespace SymbolicExecution;

/*SymbolicExecutionFringe*/
SymbolicExecutionFringe::SymbolicExecutionFringe(Reporter &r) :
        reporter(r),
        parent{},
        currentStack{},
        symbolicVarSet{}{}

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
    return failNode;
}

bool SymbolicExecutionManager::visitNode(shared_ptr<SymbolicExecutionFringe> sef, shared_ptr<CFGNode> n)
{
    string debug = n->getName();
    if (!sef->isFeasable())
    {
        return false;
    }
    else if (sef->hasSeen(n->getName()))
    {
        return true;
    }

    for (shared_ptr<AbstractCommand> command : n->getInstrs())
    {
        if (!command->acceptSymbolicExecution(sef)) return false;
    }

    feasableVisits[n->getName()]++;
    if (n->getName() == cfg.getLast()->getName()) return true;

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
            if (jocc->term2Type != JumpOnComparisonCommand::ComparitorType::ID) //comparing to a literal
            {
                if ((jocc->term2Type == JumpOnComparisonCommand::ComparitorType::DOUBLELIT && LHS->getType() != DOUBLE)
                        || (jocc->term2Type == JumpOnComparisonCommand::ComparitorType::STRINGLIT && LHS->getType() != STRING)) //todo shorten stringlit
                {
                    sef->error(Reporter::TYPE, "'" + jocc->term1 + "' (type " + TypeEnumNames[LHS->getType()]
                                               + ")  compared to a different type",
                               jocc->getLineNum());
                    return false;
                }

                string& rhs = jocc->term2;

                if (LHS->isDetermined()) return LHS->meetsConstComparison(jocc->op, rhs);

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
                        switch(jocc->op)
                        {
                            case Relations::EQ:
                                return branchEQ(sef, n, LHS->getName(), jocc->term2);
                            case Relations::NE:
                                return branchNE(sef, n, LHS->getName(), jocc->term2);
                            case Relations::LT:
                                return branchLT(sef, n, LHS->getName(), jocc->term2);
                            case Relations::LE:
                                return branchLE(sef, n, LHS->getName(), jocc->term2);
                            case Relations::GT:
                                return branchGT(sef, n, LHS->getName(), jocc->term2);
                            case Relations::GE:
                                return branchGE(sef, n, LHS->getName(), jocc->term2);
                            default:
                                throw runtime_error("bad relop");
                        }
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
                    bool meetsComparison;

                    if (RHS->isDetermined())
                    {
                        if (LHS->meetsConstComparison(jocc->op, RHS->getConstString())) return visitNode(sef, n->getCompSuccess());
                        else
                        {
                            Relations::Relop mirroredOp = Relations::mirrorRelop(jocc->op);
                            switch (RHS->canMeet(, LHS->getConstString()))
                        }
                    }

                    shared_ptr<CFGNode> nextnode = meetsComparison ? n->getCompSuccess() : getFailNode(sef, n);
                    return visitNode(sef, nextnode);
                }
                else if (RHS->isDetermined())
                {
                    shared_ptr<CFGNode> nextnode = LHS->meetsConstComparison(jocc->op, RHS->getConstString())?
                                                   n->getCompSuccess() : getFailNode(sef, n);
                    if (nextnode == nullptr) return false;
                    return visitNode(sef, nextnode);
                }

                //neither are determined here
                switch(jocc->op)
                {
                    case Relations::EQ:

                    case Relations::NE:
                        return branchNE(sef, n, LHS->getName(), jocc->term2);
                    case Relations::LT:
                        return branchLT(sef, n, LHS->getName(), jocc->term2);
                    case Relations::LE:
                        return branchLE(sef, n, LHS->getName(), jocc->term2);
                    case Relations::GT:
                        return branchGT(sef, n, LHS->getName(), jocc->term2);
                    case Relations::GE:
                        return branchGE(sef, n, LHS->getName(), jocc->term2);
                    default:
                        throw runtime_error("bad relop");
                }

            }
        }
    }
    else //unconditional jump
    {
        shared_ptr<CFGNode> retNode = getFailNode(sef, n);
        if (retNode == nullptr) return false;
        return visitNode(sef, retNode);
    }
}


//these things below will usually be called when we already have
//a ptr to the vars but we want to copy that var into the 'new scope'
//branch on const comparison
bool SymbolicExecutionManager::branchEQ(shared_ptr<SymbolicExecutionFringe> sef, shared_ptr<CFGNode> n,
                                        string lhsvar, string rhsconst)
{
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst, false);
    seflt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::LT, rhsconst);

    shared_ptr<CFGNode> failNode = getFailNode(seflt, n);
    if (failNode == nullptr) return false;
    bool feasable = visitNode(seflt, failNode);
    seflt.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    sefeq->symbolicVarSet->findVar(lhsvar)->setConstValue(rhsconst);
    sefeq->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::EQ, rhsconst);
    feasable = visitNode(sefeq, n->getCompSuccess());
    sefeq.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst, true);
    sefgt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::GT, rhsconst);
    failNode = getFailNode(sefgt, n);
    if (failNode == nullptr) return false;
    feasable = visitNode(sefgt, failNode);
    sefgt.reset();
    return feasable;
}

bool SymbolicExecutionManager::branchNE(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                        std::string lhsvar, std::string rhsconst)
{
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst, false);
    seflt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::LT, rhsconst);

    bool feasable = visitNode(seflt, n->getCompSuccess());
    seflt.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    sefeq->symbolicVarSet->findVar(lhsvar)->setConstValue(rhsconst);
    sefeq->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::EQ, rhsconst);
    shared_ptr<CFGNode> failNode = getFailNode(sefeq, n);
    if (failNode == nullptr) return false;
    feasable = visitNode(sefeq, n->getCompSuccess());
    sefeq.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst, true);
    sefgt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::GT, rhsconst);
    feasable = visitNode(sefgt, n->getCompSuccess());
    sefgt.reset();
    return feasable;
}

bool SymbolicExecutionManager::branchLT(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                        std::string lhsvar, std::string rhsconst)
{
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst, false);
    seflt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::LT, rhsconst);

    bool feasable = visitNode(seflt, n->getCompSuccess());
    seflt.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    sefge->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst);
    sefge->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::GE, rhsconst);
    shared_ptr<CFGNode> failNode = getFailNode(seflt, n);
    if (failNode == nullptr) return false;
    feasable = visitNode(sefge, failNode);
    sefge.reset();
    return feasable;
}

bool SymbolicExecutionManager::branchLE(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                        std::string lhsvar, std::string rhsconst)
{
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst);
    seflt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::LE, rhsconst);

    bool feasable = visitNode(seflt, n->getCompSuccess());
    seflt.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    sefge->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst, true);
    sefge->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::GT, rhsconst);
    shared_ptr<CFGNode> failNode = getFailNode(seflt, n);
    if (failNode == nullptr) return false;
    feasable = visitNode(sefge, failNode);
    sefge.reset();
    return feasable;
}

bool SymbolicExecutionManager::branchGT(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                        std::string lhsvar, std::string rhsconst)
{
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst, false);
    seflt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::GT, rhsconst);

    bool feasable = visitNode(seflt, n->getCompSuccess());
    seflt.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    sefge->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst);
    sefge->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::LE, rhsconst);
    shared_ptr<CFGNode> failNode = getFailNode(seflt, n);
    if (failNode == nullptr) return false;
    feasable = visitNode(sefge, failNode);
    sefge.reset();
    return feasable;
}

bool SymbolicExecutionManager::branchGE(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                        std::string lhsvar, std::string rhsconst)
{
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst);
    seflt->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::GE, rhsconst);

    bool feasable = visitNode(seflt, n->getCompSuccess());
    seflt.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    sefge->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst, true);
    sefge->pathConditions[n->getName()]
            = make_shared<Condition>(lhsvar, Relations::LT, rhsconst);
    shared_ptr<CFGNode> failNode = getFailNode(seflt, n);
    if (failNode == nullptr) return false;
    feasable = visitNode(sefge, failNode);
    sefge.reset();
    return feasable;
}

//branching on var comparison
//todo check below (RHS lower vs upper) and finish the rest
//todo path conditions
template <typename T>
bool SymbolicExecutionManager::varBranchGE(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                           VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar)
{
    string LHSLowerBound = to_string(lhsvar->getLowerBound());
    string LHSUpperBound = to_string(lhsvar->getUpperBound());
    string RHSLowerBound = to_string(rhsvar->getLowerBound());
    string RHSUpperBound = to_string(rhsvar->getUpperBound());

    shared_ptr<CFGNode> failNode = getFailNode(sef, n);
    if (failNode == nullptr) return false;

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVar(lhsvar->getName())->setUpperBound(RHSLowerBound, false);
    bool feasable = visitNode(seflt, failNode);
    seflt.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    sefge->symbolicVarSet->findVar(lhsvar->getName())->setLowerBound(RHSLowerBound);
    feasable = visitNode(sefge, n->getCompSuccess());
    sefge.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::varBranchGT(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                              VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar)
{
    string LHSLowerBound = to_string(lhsvar->getLowerBound());
    string LHSUpperBound = to_string(lhsvar->getUpperBound());
    string RHSLowerBound = to_string(rhsvar->getLowerBound());
    string RHSUpperBound = to_string(rhsvar->getUpperBound());

    shared_ptr<CFGNode> failNode = getFailNode(sef, n);
    if (failNode == nullptr) return false;

    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    sefle->symbolicVarSet->findVar(lhsvar->getName())->setUpperBound(RHSLowerBound);
    bool feasable = visitNode(sefle, failNode);
    sefle.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVar(lhsvar->getName())->setLowerBound(RHSLowerBound);
    feasable = visitNode(sefgt, n->getCompSuccess());
    sefgt.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::varBranchLT(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                              VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar)
{
    string LHSLowerBound = to_string(lhsvar->getLowerBound());
    string LHSUpperBound = to_string(lhsvar->getUpperBound());
    string RHSLowerBound = to_string(rhsvar->getLowerBound());
    string RHSUpperBound = to_string(rhsvar->getUpperBound());

    shared_ptr<CFGNode> failNode = getFailNode(sef, n);
    if (failNode == nullptr) return false;

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVar(lhsvar->getName())->setUpperBound(RHSLowerBound, false);
    bool feasable = visitNode(seflt, n->getCompSuccess());
    seflt.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    sefge->symbolicVarSet->findVar(lhsvar->getName())->setLowerBound(RHSLowerBound);
    feasable = visitNode(sefge, failNode);
    sefge.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::varBranchLE(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                              VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar)
{
    string LHSLowerBound = to_string(lhsvar->getLowerBound());
    string LHSUpperBound = to_string(lhsvar->getUpperBound());
    string RHSLowerBound = to_string(rhsvar->getLowerBound());
    string RHSUpperBound = to_string(rhsvar->getUpperBound());

    shared_ptr<CFGNode> failNode = getFailNode(sef, n);
    if (failNode == nullptr) return false;

    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    sefle->symbolicVarSet->findVar(lhsvar->getName())->setUpperBound(RHSLowerBound);
    bool feasable = visitNode(sefle, n->getCompSuccess());
    sefle.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVar(lhsvar->getName())->setLowerBound(RHSLowerBound);
    feasable = visitNode(sefgt, n->getCompSuccess());
    sefgt.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::varBranchNE(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                           VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar)
{
    string LHSLowerBound = to_string(lhsvar->getLowerBound());
    string LHSUpperBound = to_string(lhsvar->getUpperBound());
    string RHSLowerBound = to_string(rhsvar->getLowerBound());
    string RHSUpperBound = to_string(rhsvar->getUpperBound());

    shared_ptr<CFGNode> failNode = getFailNode(sef, n);
    if (failNode == nullptr) return false;

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVar(lhsvar->getName())->setLowerBound(RHSUpperBound, false);
    bool feasable = visitNode(sefgt, n->getCompSuccess());
    sefgt.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVar(lhsvar->getName())->setUpperBound(RHSLowerBound, true);
    feasable = visitNode(sefgt, n->getCompSuccess());
    sefgt.reset();
    if (!feasable) return false;

    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    //T upperBound = ()

    sefgt->symbolicVarSet->findVar(lhsvar->getName())->setUpperBound(RHSLowerBound, true);
    feasable = visitNode(sefgt, n->getCompSuccess());
    sefgt.reset();
    if (!feasable) return false;
}

template <typename T>
bool SymbolicExecutionManager::varBranchEQ(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                                           VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar)
{

}
