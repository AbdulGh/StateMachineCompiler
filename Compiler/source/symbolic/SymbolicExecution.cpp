#include <algorithm>

//todo refactor (lmoa)

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

SymbolicExecutionFringe::SymbolicExecutionFringe(shared_ptr<SymbolicExecutionFringe> p):
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

string SymbolicExecutionFringe::printPathConditions()
{
    string out;
    if (parent != nullptr) out = parent->printPathConditions();
    for (string& nodeName : visitOrder)
    {
        out += "Visit " + nodeName + " - branch " + pathConditions.at(nodeName).toString() + "\n";
    }
    return out;
}

void SymbolicExecutionFringe::addPathCondition(const std::string& nodeName, JumpOnComparisonCommand* jocc, bool negate)
{
    if (hasSeen(nodeName)) throw "can't visit twice";
    visitOrder.push_back(nodeName);
    pathConditions.insert({nodeName, Condition(jocc->term1, negate ? Relations::negateRelop(jocc->op) : jocc->op, jocc->term2)});
    auto symvar = symbolicVarSet->findVar(jocc->term1);
    if (symvar == nullptr) throw "comparing unknown var";
    bool closed = false;
    switch(jocc->op)
    {
        case Relations::LT:
            closed = true;
        case Relations::LE:
            symvar->clipUpperBound(jocc->term2, closed);
            break;
        case Relations::GT:
            closed = true;
        case Relations::GE:
            symvar->clipLowerBound(jocc->term2, closed);
            break;
        default:
            throw "todo";
    }
}

/*SymbolicExecutionManager*/
void SymbolicExecutionManager::search()
{
    feasableVisits.clear();
    shared_ptr<SymbolicExecutionFringe> sef = make_shared<SymbolicExecutionFringe>(reporter);
    visitNode(sef, cfg.getFirst());
    for (auto& p : feasableVisits)
    {
        if (p.second == 0) // no feasable visits - remove
        {
            reporter.optimising(Reporter::DEADCODE, "State '" + p.first + "' is unreachable and will be removed");
            CFGNode* lonelyNode = cfg.getNode(p.first);
            if (lonelyNode->getCompSuccess() != nullptr) lonelyNode->getCompSuccess()->removeParent(lonelyNode);
            if (lonelyNode->getCompFail() != nullptr) lonelyNode->getCompFail()->removeParent(lonelyNode);
            for (auto& parentPair : lonelyNode->getPredecessorMap())
            {
                CFGNode* parent = parentPair.second;
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

CFGNode*
SymbolicExecutionManager::getFailNode(shared_ptr<SymbolicExecutionFringe> returningSEF, CFGNode* n)
{
    CFGNode* failNode = n->getCompFail();
    if (failNode == nullptr) //return to top of stack
    {
        if (!n->isLastNode()) throw "only last can return";
        if (returningSEF->currentStack->getTopType() != SymbolicStackMemberType::STATE)
        {
            returningSEF->error(Reporter::BAD_STACK_USE, "Tried to jump to a non state", n->getJumpline()); //probably my fault
            return nullptr;
        }
        failNode = n->getParentGraph().getNode(returningSEF->currentStack->popState());
        if (failNode == nullptr)
        {
            returningSEF->error(Reporter::BAD_STACK_USE, "Tried to jump to a nonexisting state", n->getJumpline());
            return nullptr;
        }
        else //check its in successors of n
        if (find(n->getSuccessorVector().begin(), n->getSuccessorVector().end(), failNode) == n->getSuccessorVector().end())
        {
            throw "should be successor";
        }
    }
    return failNode;
}

bool SymbolicExecutionManager::visitNode(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n)
{
    if (!sef->isFeasable()) return false;
    else if (sef->hasSeen(n->getName())) return true;

    for (const auto& command : n->getInstrs())
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

    JumpOnComparisonCommand* jocc = n->getComp();
    if (jocc != nullptr) //is a conditional jump
    {
        //const comparisons were caught during compilation
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

            if (LHS->isDetermined() && LHS->meetsConstComparison(jocc->op, rhs)) return visitNode(sef, n->getCompSuccess());

            switch (LHS->canMeet(jocc->op, rhs))
            {
                case SymbolicVariable::CANT:
                {
                    CFGNode* nextNode = getFailNode(sef, n);
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
                    CFGNode* nextNode = getFailNode(sef, n);
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
                    if (LHS->meetsConstComparison(jocc->op, RHS->getConstString())) return visitNode(sef, n->getCompSuccess());
                    else
                    {
                        CFGNode* nextnode = getFailNode(sef, n);
                        if (nextnode == nullptr) return false;
                        return visitNode(sef, nextnode);
                    }

                }
                else //rhs undetermined, lhs determined
                {
                    Relations::Relop mirroredOp = Relations::mirrorRelop(jocc->op);
                    return branchOnType(sef, RHS->getName(), mirroredOp,
                                                       LHS->getConstString(), RHS->getType(), true);
                }
            }
            else if (RHS->isDetermined())
            {
                return branchOnType(sef, LHS->getName(), jocc->op, RHS->getConstString(), LHS->getType(), false);
            }

            //neither are determined here
            if (LHS->getType() == STRING)
            {
                VarTemplatePointer<string> LHS = static_pointer_cast<SymbolicVariableTemplate<string>>(LHS);
                VarTemplatePointer<string> RHS = static_pointer_cast<SymbolicVariableTemplate<string>>(RHS);
                return varBranch<string>(sef, n, LHS, jocc->op, RHS);
            }
            else if (LHS->getType() == DOUBLE)
            {
                VarTemplatePointer<double> LHS = static_pointer_cast<SymbolicVariableTemplate<double>>(LHS);
                VarTemplatePointer<double> RHS = static_pointer_cast<SymbolicVariableTemplate<double>>(RHS);
                return varBranch<double>(sef, n, LHS, jocc->op, RHS);
            }
        }
    }
    else //unconditional jump
    {
        if (n->getName() == cfg.getLast()->getName()) return true;
        CFGNode* retNode = getFailNode(sef, n);
        if (retNode == nullptr) return false;
        return visitNode(sef, retNode);
    }
}

//these things below will usually be called when we already have
//a ptr to the vars but we want to copy that var into the 'new scope'
template <typename T>
bool SymbolicExecutionManager::branch(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n, string lhsvar,
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


#define visitWithFeasable(sef, n) if (visitNode(sef, n)) feasable = true;
template <typename T>
bool SymbolicExecutionManager::branchEQ(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        string lhsvar, const T& rhsconst, bool reverse)
{
    bool feasable = false;

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVarOfType<T>(lhsvar)->setTUpperBound(rhsconst, false);

    if (reverse) {visitWithFeasable(seflt, n->getCompSuccess());}
    else
    {
        CFGNode* failNode = getFailNode(seflt, n);
        if (failNode == nullptr)
        {
            return false;
        }
        visitWithFeasable(seflt, failNode);
    }
    seflt.reset();

    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    sefeq->symbolicVarSet->findVarOfType<T>(lhsvar)->setConstValue(rhsconst);
    if (reverse)
    {
        CFGNode* failNode = getFailNode(sefeq, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(sefeq, failNode);
    }
    else  visitWithFeasable(sefeq, n->getCompSuccess());

    sefeq.reset();

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVarOfType<T>(lhsvar)->setTLowerBound(rhsconst, true);

    if (reverse) {visitWithFeasable(sefgt, n->getCompSuccess());}
    else
    {
        CFGNode* failNode = getFailNode(sefgt, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(sefgt, failNode);
    }

    sefgt.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::branchNE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        string lhsvar, const T& rhsconst, bool reverse)
{
    bool feasable = false;
    
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVarOfType<T>(lhsvar)->setTUpperBound(rhsconst, false);
    if (reverse)
    {
        CFGNode* failNode = getFailNode(seflt, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(seflt, failNode);
    }
    else visitWithFeasable(seflt, n->getCompSuccess());
    seflt.reset();
    
    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    sefeq->symbolicVarSet->findVarOfType<T>(lhsvar)->setConstValue(rhsconst);
    if (reverse) {visitWithFeasable(seflt, n->getCompSuccess());}
    else
    {
        CFGNode* failNode = getFailNode(sefeq, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(sefeq, failNode);
    }
    sefeq.reset();
    

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVarOfType<T>(lhsvar)->setTLowerBound(rhsconst, true);
    if (reverse)
    {
        CFGNode* failNode = getFailNode(seflt, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(seflt, failNode);
    }
    else visitWithFeasable(seflt, n->getCompSuccess());
    sefgt.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::branchLT(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        string lhsvar, const T& rhsconst, bool reverse)
{
    bool feasable = false;
    
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVarOfType<T>(lhsvar)->setTUpperBound(rhsconst, false);

    if (reverse)
    {
        CFGNode* failNode = getFailNode(seflt, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(seflt, failNode);
    }
    else visitWithFeasable(seflt, n->getCompSuccess());
    seflt.reset();
    
    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    sefge->symbolicVarSet->findVarOfType<T>(lhsvar)->setTLowerBound(rhsconst);
    if (reverse) {visitWithFeasable(sefge, n->getCompSuccess());}
    else
    {
        CFGNode* failNode = getFailNode(sefge, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(sefge, failNode);
    }
    sefge.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::branchLE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        string lhsvar, const T& rhsconst, bool reverse)
{
    bool feasable = false;

    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    sefle->symbolicVarSet->findVarOfType<T>(lhsvar)->setTUpperBound(rhsconst);

    if (reverse)
    {
        CFGNode* failNode = getFailNode(sefle, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(sefle, failNode);
    }
    else visitWithFeasable(sefle, n->getCompSuccess());
    sefle.reset();
    

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVarOfType<T>(lhsvar)->setTLowerBound(rhsconst, true);
    if (reverse) {visitWithFeasable(sefgt, n->getCompSuccess());}
    else
    {
        CFGNode* failNode = getFailNode(sefgt, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(sefgt, failNode);
    }
    sefgt.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::branchGT(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        string lhsvar, const T& rhsconst, bool reverse)
{
    bool feasable = false;

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVarOfType<T>(lhsvar)->setTLowerBound(rhsconst, false);

    if (reverse)
    {
        CFGNode* failNode = getFailNode(sefgt, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(sefgt, failNode);
    }
    else visitWithFeasable(sefgt, n->getCompSuccess());
    sefgt.reset();
    

    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    sefle->symbolicVarSet->findVarOfType<T>(lhsvar)->setTUpperBound(rhsconst);

    if (reverse) {visitWithFeasable(sefle, n->getCompSuccess());}
    else
    {
        CFGNode* failNode = getFailNode(sefle, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(sefle, failNode);
    }
    sefle.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::branchGE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        string lhsvar, const T& rhsconst, bool reverse)
{
    bool feasable = false;

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    sefge->symbolicVarSet->findVarOfType<T>(lhsvar)->setTLowerBound(rhsconst);

    if (reverse)
    {
        CFGNode* failNode = getFailNode(sefge, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(sefge, failNode);
    }
    else visitWithFeasable(sefge, n->getCompSuccess());
    sefge.reset();
    

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVarOfType<T>(lhsvar)->setTUpperBound(rhsconst, true);
    if (reverse) {visitWithFeasable(seflt, n->getCompSuccess());}
    else
    {
        CFGNode* failNode = getFailNode(seflt, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(seflt, failNode);
    }
    seflt.reset();
    return feasable;
}

//branching on var comparison
template <typename T>
bool SymbolicExecutionManager::varBranch(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
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
bool SymbolicExecutionManager::varBranchGE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                           VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar)
{
    bool feasable = false;
    
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    CFGNode* failNode = getFailNode(seflt, n);
    if (failNode == nullptr) return false;
    if (seflt->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipTUpperBound(rhsvar->getUpperBound(), false)) return false;
    visitWithFeasable(seflt, failNode);
    seflt.reset();

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    if (!sefge->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipTLowerBound(rhsvar->getLowerBound())) return false;
    visitWithFeasable(sefge, n->getCompSuccess());
    sefge.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::varBranchGT(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                              VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar)
{
    bool feasable = false;
    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    CFGNode* failNode = getFailNode(sefle, n);
    if (failNode == nullptr) return false;
    if (!sefle->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipTUpperBound(rhsvar->getUpperBound())) return false;
    visitWithFeasable(sefle, failNode);
    sefle.reset();
    

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    if (!sefgt->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipTLowerBound(rhsvar->getLowerBound(), false)) return false;
    visitWithFeasable(sefgt, n->getCompSuccess());
    sefgt.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::varBranchLT(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                              VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar)
{

    bool feasable = false;
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    CFGNode* failNode = getFailNode(seflt, n);
    if (failNode == nullptr) return false;
    if (!seflt->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipTUpperBound(rhsvar->getUpperBound(), false)) return false;
    visitWithFeasable(seflt, n->getCompSuccess());
    seflt.reset();
    

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    if (!sefge->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipTLowerBound(rhsvar->getLowerBound())) return false;
    visitWithFeasable(sefge, failNode);
    sefge.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::varBranchLE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                              VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar)
{
    bool feasable = false;

    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    CFGNode* failNode = getFailNode(sefle, n);
    if (failNode == nullptr) return false;
    if (!sefle->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipTUpperBound(rhsvar->getUpperBound())) return false;
    visitWithFeasable(sefle, n->getCompSuccess());
    sefle.reset();
    

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    if (!sefgt->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipTLowerBound(rhsvar->getLowerBound())) return false;
    visitWithFeasable(sefgt, n->getCompSuccess());
    sefgt.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::varBranchNE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                           VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar)
{
    bool feasable = false;

    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    CFGNode* failNode = getFailNode(sefeq, n);
    if (failNode == nullptr) return false;
    VarTemplatePointer<T>& glbvar = getGreatestLowerBound(lhsvar, rhsvar);
    if (glbvar->isBoundedBelow())
    {
        //pull the variables into sefeq
        VarTemplatePointer<T> lhsvar = sefeq->symbolicVarSet->findVarOfType<T>(lhsvar->getName());
        VarTemplatePointer<T> rhsvar = sefeq->symbolicVarSet->findVarOfType<T>(rhsvar->getName());

        const T& glb = glbvar->getLowerBound();
        if (!lhsvar->clipTLowerBound(glb) || !rhsvar->clipTLowerBound(glb)) return false;
    }
    VarTemplatePointer<T>& lubvar = getLeastUpperBound(lhsvar, rhsvar);
    if (lubvar->isBoundedAbove())
    {
        VarTemplatePointer<T> lhsvar = sefeq->symbolicVarSet->findVarOfType<T>(lhsvar->getName());
        VarTemplatePointer<T> rhsvar = sefeq->symbolicVarSet->findVarOfType<T>(rhsvar->getName());

        const T& lub = lubvar->getUpperBound();
        if (!lhsvar->clipTUpperBound(lub) || !rhsvar->clipTUpperBound(lub)) return false;
    }
    visitWithFeasable(sefeq, failNode);

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    if (!sefgt->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipTLowerBound(rhsvar->getUpperBound(), false)) return false;
    visitWithFeasable(sefgt, n->getCompSuccess());
    sefgt.reset();

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    if (!seflt->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipTUpperBound(rhsvar->getLowerBound(), true)) return false;
    visitWithFeasable(seflt, n->getCompSuccess());
    seflt.reset();
    return feasable;
}

template <typename T>
bool SymbolicExecutionManager::varBranchEQ(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                           VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar)
{
    bool feasable = false;

    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    VarTemplatePointer<T>& glbvar = getGreatestLowerBound(lhsvar, rhsvar);
    if (glbvar->isBoundedBelow())
    {
        //pull the variables into sefeq
        VarTemplatePointer<T> lhsvar = sefeq->symbolicVarSet->findVarOfType<T>(lhsvar->getName());
        VarTemplatePointer<T> rhsvar = sefeq->symbolicVarSet->findVarOfType<T>(rhsvar->getName());

        const T& glb = glbvar->getLowerBound();
        if (!lhsvar->clipTLowerBound(glb) || !rhsvar->clipTLowerBound(glb)) return false;
    }
    VarTemplatePointer<T>& lubvar = getLeastUpperBound(lhsvar, rhsvar);
    if (lubvar->isBoundedAbove())
    {
        VarTemplatePointer<T> lhsvar = sefeq->symbolicVarSet->findVarOfType<T>(lhsvar->getName());
        VarTemplatePointer<T> rhsvar = sefeq->symbolicVarSet->findVarOfType<T>(rhsvar->getName());

        const T& lub = lubvar->getUpperBound();
        if (!lhsvar->clipTUpperBound(lub) || !rhsvar->clipTUpperBound(lub)) return false;
    }
    visitWithFeasable(sefeq, n->getCompSuccess());

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    CFGNode* failNode = getFailNode(sefgt, n);
    if (failNode == nullptr) return false;
    if (!sefgt->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipTLowerBound(rhsvar->getUpperBound(), false)) return false;
    visitWithFeasable(sefgt, n->getCompSuccess());
    sefgt.reset();

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    if (!seflt->symbolicVarSet->findVarOfType<T>(lhsvar->getName())->clipTUpperBound(rhsvar->getLowerBound(), true)) return false;
    failNode = getFailNode(sef, n);
    if (failNode == nullptr) return false;
    visitWithFeasable(seflt, failNode);
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