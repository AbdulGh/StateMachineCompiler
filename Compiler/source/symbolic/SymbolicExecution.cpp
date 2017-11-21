#include <algorithm>

#include "SymbolicExecution.h"
#include "../Command.h"

using namespace std;
using namespace SymbolicExecution;

//SymbolicExecutionFringe
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
    if (pathConditions.find(state) != pathConditions.end()) return true;
    else return (parent == nullptr) ? false : parent->hasSeen(state);
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
    Relations::Relop op = negate ? Relations::negateRelop(jocc->op) : jocc->op;
    pathConditions.insert({nodeName, Condition(jocc->term1, op, jocc->term2)});
    auto t1var = symbolicVarSet->findVar(jocc->term1);
    if (t1var == nullptr) throw "comparing unknown var or constants";
    if (jocc->term2Type == AbstractCommand::StringType::ID)
    {
        auto t2var = symbolicVarSet->findVar(jocc->term2);
        if (t2var == nullptr) throw "comparing unknown var";
        switch(op)
        {
            case Relations::LT:
                t1var->addLT(t2var); break;
            case Relations::LE:
                t1var->addLE(t2var); break;
            case Relations::GT:
                t1var->addGT(t2var); break;
            case Relations::GE:
                t1var->addGE(t2var); break;
            case Relations::EQ:
                t1var->addEQ(t2var); break;
            case Relations::NE:
                t1var->addNEQ(t2var); break;
            default:
                throw "unknown op";
        }
    }
    else
    {
        bool closed = false;
        switch(op)
        {
            case Relations::LT:
                closed = true;
            case Relations::LE:
                t1var->clipUpperBound(jocc->term2, closed);
                break;
            case Relations::GT:
                closed = true;
            case Relations::GE:
                t1var->clipLowerBound(jocc->term2, closed);
                break;
            case Relations::EQ:
                t1var->setConstValue(jocc->term2);
                break;
            case Relations::NE:
                t1var->addNEQConst(jocc->term2);
                break;
            default:
                throw "unknown relop";
        }
    }
}

//SymbolicExecutionManager
unordered_map<string, shared_ptr<SymbolicVarSet>>& SymbolicExecutionManager::search()
{
    visitedNodes.clear();
    tags.clear();
    for (auto& pair : cfg.getCurrentNodes()) tags[pair.first] = make_shared<SymbolicVarSet>();
    shared_ptr<SymbolicExecutionFringe> sef = make_shared<SymbolicExecutionFringe>(reporter);
    visitNode(sef, cfg.getFirst());
    auto it = cfg.getCurrentNodes().begin();
    while (it != cfg.getCurrentNodes().end())
    {
        if (visitedNodes.find(it->first) == visitedNodes.end()) // no feasable visits - remove
        {
            reporter.optimising(Reporter::DEADCODE, "State '" + it->first + "' is unreachable and will be removed");
            CFGNode* lonelyNode = it->second.get();
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
            it = cfg.removeNode(it->first);
        }
        else ++it;
    }
    return tags;
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

    sef->pathConditions.insert({n->getName(), Condition()}); //don't track conditions till later - this just tracks which nodes we've seen
    tags[n->getName()]->unionSVS(sef->symbolicVarSet.get());

    for (const auto& command : n->getInstrs())
    {
        //might be in a loop
        if (command->getType() == CommandType::EXPR)
        {
            EvaluateExprCommand* eec = static_cast<EvaluateExprCommand*>(command.get());
            if (eec->op == MOD)
            {
                try
                {
                    stod(eec->term2);
                    if (!command->acceptSymbolicExecution(sef)) return false;
                }
                catch (invalid_argument&)
                {
                    sef->symbolicVarSet->findVar(command->getData())->userInput();
                }
            }
            else sef->symbolicVarSet->findVar(command->getData())->userInput();
        }
        else if (!command->acceptSymbolicExecution(sef)) return false;
    }

    visitedNodes.insert(n->getName());

    if (n->getName() == n->getParentGraph().getLast()->getName()) return true;

    JumpOnComparisonCommand* jocc = n->getComp();
    if (jocc != nullptr) //is a conditional jump
    {
        //const comparisons were caught during compilation
        //note: JOCC constructor ensures that if there is a var there is a var on the LHS
        SymbolicVariable* LHS = sef->symbolicVarSet->findVar(jocc->term1);
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
                    return branch(sef, n, LHS->getName(), jocc->op, rhs);
                }
                case SymbolicVariable::MUST:
                {
                    return visitNode(sef, n->getCompSuccess());
                }
                default:
                    throw runtime_error("very weird enum");
            }
        }
        else //comparing to another variable
        {
            SymbolicVariable* RHS = sef->symbolicVarSet->findVar(jocc->term2);
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
                    return branch(sef, n, RHS->getName(), mirroredOp, LHS->getConstString(), true);
                }
            }
            else if (RHS->isDetermined())
            {
                return branch(sef, n, LHS->getName(), jocc->op, RHS->getConstString(), false);
            }

            //neither are determined here
            return varBranch(sef, n, LHS, jocc->op, RHS);
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

bool SymbolicExecutionManager::branch(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n, string lhsvar,
                                      Relations::Relop op, const std::string& rhsconst, bool reverse)
{
    switch(op)
    {
        case Relations::EQ:
            return branchEQ(sef, n, lhsvar, rhsconst, reverse);
        case Relations::NE:
            return branchNE(sef, n, lhsvar, rhsconst, reverse);
        case Relations::LT:
            return branchLT(sef, n, lhsvar, rhsconst, reverse);
        case Relations::LE:
            return branchLE(sef, n, lhsvar, rhsconst, reverse);
        case Relations::GT:
            return branchGT(sef, n, lhsvar, rhsconst, reverse);
        case Relations::GE:
            return branchGE(sef, n, lhsvar, rhsconst, reverse);
        default:
            throw runtime_error("bad relop");
    }
}

#define visitWithFeasable(sef, n) if (visitNode(sef, n)) feasable = true;

bool SymbolicExecutionManager::branchEQ(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        string lhsvar, const std::string& rhsconst, bool reverse)
{
    bool feasable = false;

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst, false);
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
    sefeq->symbolicVarSet->findVar(lhsvar)->setConstValue(rhsconst);
    if (reverse)
    {
        CFGNode* failNode = getFailNode(sefeq, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(sefeq, failNode);
    }
    else  visitWithFeasable(sefeq, n->getCompSuccess());

    sefeq.reset();

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst, true);

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


bool SymbolicExecutionManager::branchNE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        string lhsvar, const std::string& rhsconst, bool reverse)
{
    bool feasable = false;
    
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst, false);
    if (reverse)
    {
        CFGNode* failNode = getFailNode(seflt, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(seflt, failNode);
    }
    else visitWithFeasable(seflt, n->getCompSuccess());
    seflt.reset();
    
    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    sefeq->symbolicVarSet->findVar(lhsvar)->setConstValue(rhsconst);
    if (reverse) {visitWithFeasable(seflt, n->getCompSuccess());}
    else
    {
        CFGNode* failNode = getFailNode(sefeq, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(sefeq, failNode);
    }
    sefeq.reset();
    

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst, true);
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


bool SymbolicExecutionManager::branchLT(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        string lhsvar, const std::string& rhsconst, bool reverse)
{
    bool feasable = false;
    
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst, false);

    if (reverse)
    {
        CFGNode* failNode = getFailNode(seflt, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(seflt, failNode);
    }
    else visitWithFeasable(seflt, n->getCompSuccess());
    seflt.reset();
    
    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    sefge->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst);
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


bool SymbolicExecutionManager::branchLE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        string lhsvar, const std::string& rhsconst, bool reverse)
{
    bool feasable = false;

    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    sefle->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst);

    if (reverse)
    {
        CFGNode* failNode = getFailNode(sefle, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(sefle, failNode);
    }
    else visitWithFeasable(sefle, n->getCompSuccess());
    sefle.reset();
    

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst, true);
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


bool SymbolicExecutionManager::branchGT(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        string lhsvar, const std::string& rhsconst, bool reverse)
{
    bool feasable = false;

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst, false);
    if (reverse)
    {
        CFGNode* failNode = getFailNode(sefgt, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(sefgt, failNode);
    }
    else visitWithFeasable(sefgt, n->getCompSuccess());
    sefgt.reset();
    

    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    sefle->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst);

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


bool SymbolicExecutionManager::branchGE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        string lhsvar, const std::string& rhsconst, bool reverse)
{
    bool feasable = false;

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    sefge->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst);

    if (reverse)
    {
        CFGNode* failNode = getFailNode(sefge, n);
        if (failNode == nullptr) return false;
        visitWithFeasable(sefge, failNode);
    }
    else visitWithFeasable(sefge, n->getCompSuccess());
    sefge.reset();
    

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst, true);
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
bool SymbolicExecutionManager::varBranch(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                         SymbolicVariable* LHS, Relations::Relop op,
                                         SymbolicVariable* RHS)
{
    switch(op)
    {
        case Relations::EQ:
            return varBranchEQ(sef, n, LHS, RHS);
        case Relations::NE:
            return varBranchNE(sef, n, LHS, RHS);
        case Relations::LT:
            return varBranchLT(sef, n, LHS, RHS);
        case Relations::LE:
            return varBranchLE(sef, n, LHS, RHS);
        case Relations::GT:
            return varBranchGT(sef, n, LHS, RHS);
        case Relations::GE:
            return varBranchGE(sef, n, LHS, RHS);
        default:
            throw runtime_error("bad relop");
    }
}


bool SymbolicExecutionManager::varBranchGE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                           SymbolicVariable* lhsvar, SymbolicVariable* rhsvar)
{
    bool feasable = false;
    
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    CFGNode* failNode = getFailNode(seflt, n);
    if (failNode == nullptr) return false;
    if (rhsvar->isBoundedAbove() &&
            !seflt->symbolicVarSet->findVar(lhsvar->getName())->clipUpperBound(rhsvar->getUpperBound(), false)) return false;
    visitWithFeasable(seflt, failNode);
    seflt.reset();

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    if (rhsvar->isBoundedBelow() &&
            !sefge->symbolicVarSet->findVar(lhsvar->getName())->clipLowerBound(rhsvar->getLowerBound())) return false;
    visitWithFeasable(sefge, n->getCompSuccess());
    sefge.reset();
    return feasable;
}


bool SymbolicExecutionManager::varBranchGT(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                              SymbolicVariable* lhsvar, SymbolicVariable* rhsvar)
{
    bool feasable = false;
    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    CFGNode* failNode = getFailNode(sefle, n);
    if (failNode == nullptr) return false;
    if (rhsvar->isBoundedAbove() &&
            !sefle->symbolicVarSet->findVar(lhsvar->getName())->clipUpperBound(rhsvar->getUpperBound())) return false;
    visitWithFeasable(sefle, failNode);
    sefle.reset();
    

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    if (rhsvar->isBoundedBelow() &&
            !sefgt->symbolicVarSet->findVar(lhsvar->getName())->clipLowerBound(rhsvar->getLowerBound(), false)) return false;
    visitWithFeasable(sefgt, n->getCompSuccess());
    sefgt.reset();
    return feasable;
}

bool SymbolicExecutionManager::varBranchLT(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                              SymbolicVariable* lhsvar, SymbolicVariable* rhsvar)
{

    bool feasable = false;
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    CFGNode* failNode = getFailNode(seflt, n);
    if (failNode == nullptr) return false;
    if (rhsvar->isBoundedAbove() &&
            !seflt->symbolicVarSet->findVar(lhsvar->getName())->clipUpperBound(rhsvar->getUpperBound(), false)) return false;
    visitWithFeasable(seflt, n->getCompSuccess());
    seflt.reset();
    

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    if (rhsvar->isBoundedBelow() &&
            !sefge->symbolicVarSet->findVar(lhsvar->getName())->clipLowerBound(rhsvar->getLowerBound())) return false;
    visitWithFeasable(sefge, failNode);
    sefge.reset();
    return feasable;
}


bool SymbolicExecutionManager::varBranchLE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                              SymbolicVariable* lhsvar, SymbolicVariable* rhsvar)
{
    bool feasable = false;

    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    CFGNode* failNode = getFailNode(sefle, n);
    if (failNode == nullptr) return false;
    if (rhsvar->isBoundedAbove() &&
            !sefle->symbolicVarSet->findVar(lhsvar->getName())->clipUpperBound(rhsvar->getUpperBound())) return false;
    visitWithFeasable(sefle, n->getCompSuccess());
    sefle.reset();
    

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    if (rhsvar->isBoundedBelow() &&
            !sefgt->symbolicVarSet->findVar(lhsvar->getName())->clipLowerBound(rhsvar->getLowerBound())) return false;
    visitWithFeasable(sefgt, n->getCompSuccess());
    sefgt.reset();
    return feasable;
}


bool SymbolicExecutionManager::varBranchNE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                           SymbolicVariable* lhsvar, SymbolicVariable* rhsvar)
{
    bool feasable = false;

    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    CFGNode* failNode = getFailNode(sefeq, n);
    if (failNode == nullptr) return false;
    SymbolicVariable* glbvar = getGreatestLowerBound(lhsvar, rhsvar);
    if (glbvar->isBoundedBelow())
    {
        //pull the variables into sefeq
        SymbolicVariable* lhsvar = sefeq->symbolicVarSet->findVar(lhsvar->getName());
        SymbolicVariable* rhsvar = sefeq->symbolicVarSet->findVar(rhsvar->getName());

        const string& glb = glbvar->getLowerBound();
        if (!lhsvar->clipLowerBound(glb) || !rhsvar->clipLowerBound(glb)) return false;
    }
    SymbolicVariable* lubvar = getLeastUpperBound(lhsvar, rhsvar);
    if (lubvar->isBoundedAbove())
    {
        SymbolicVariable* lhsvar = sefeq->symbolicVarSet->findVar(lhsvar->getName());
        SymbolicVariable* rhsvar = sefeq->symbolicVarSet->findVar(rhsvar->getName());

        const string& lub = lubvar->getUpperBound();
        if (!lhsvar->clipUpperBound(lub) || !rhsvar->clipUpperBound(lub)) return false;
    }
    visitWithFeasable(sefeq, failNode);

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    if (!sefgt->symbolicVarSet->findVar(lhsvar->getName())->clipLowerBound(rhsvar->getUpperBound(), false)) return false;
    visitWithFeasable(sefgt, n->getCompSuccess());
    sefgt.reset();

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    if (!seflt->symbolicVarSet->findVar(lhsvar->getName())->clipUpperBound(rhsvar->getLowerBound(), true)) return false;
    visitWithFeasable(seflt, n->getCompSuccess());
    seflt.reset();
    return feasable;
}


bool SymbolicExecutionManager::varBranchEQ(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                           SymbolicVariable* lhsvar, SymbolicVariable* rhsvar)
{
    bool feasable = false;

    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    SymbolicVariable* glbvar = getGreatestLowerBound(lhsvar, rhsvar);
    if (glbvar->isBoundedBelow())
    {
        //pull the variables into sefeq
        SymbolicVariable* lhsvar = sefeq->symbolicVarSet->findVar(lhsvar->getName());
        SymbolicVariable* rhsvar = sefeq->symbolicVarSet->findVar(rhsvar->getName());

        const string& glb = glbvar->getLowerBound();
        if (!lhsvar->clipLowerBound(glb) || !rhsvar->clipLowerBound(glb)) return false;
    }
    SymbolicVariable* lubvar = getLeastUpperBound(lhsvar, rhsvar);
    if (lubvar->isBoundedAbove())
    {
        SymbolicVariable* lhsvar = sefeq->symbolicVarSet->findVar(lhsvar->getName());
        SymbolicVariable* rhsvar = sefeq->symbolicVarSet->findVar(rhsvar->getName());

        const string& lub = lubvar->getUpperBound();
        if (!lhsvar->clipUpperBound(lub) || !rhsvar->clipUpperBound(lub)) return false;
    }
    visitWithFeasable(sefeq, n->getCompSuccess());

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    CFGNode* failNode = getFailNode(sefgt, n);
    if (failNode == nullptr) return false;
    if (!sefgt->symbolicVarSet->findVar(lhsvar->getName())->clipLowerBound(rhsvar->getUpperBound(), false)) return false;
    visitWithFeasable(sefgt, n->getCompSuccess());
    sefgt.reset();

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    if (!seflt->symbolicVarSet->findVar(lhsvar->getName())->clipUpperBound(rhsvar->getLowerBound(), true)) return false;
    failNode = getFailNode(sef, n);
    if (failNode == nullptr) return false;
    visitWithFeasable(seflt, failNode);
    seflt.reset();
    return feasable;
}


SymbolicVariable* SymbolicExecutionManager::getGreatestLowerBound(SymbolicVariable* lhsvar, SymbolicVariable* rhsvar)
{
    if (!lhsvar->isBoundedBelow()) return rhsvar;
    else if (!rhsvar->isBoundedBelow()) return lhsvar;
    else return (lhsvar->getLowerBound() > rhsvar->getLowerBound()) ? lhsvar : rhsvar;
}


SymbolicVariable* SymbolicExecutionManager::getLeastUpperBound(SymbolicVariable* lhsvar, SymbolicVariable* rhsvar)
{
    if (!lhsvar->isBoundedAbove()) return rhsvar;
    else if (!rhsvar->isBoundedAbove()) return lhsvar;
    else return (lhsvar->getUpperBound() < rhsvar->getUpperBound()) ? lhsvar : rhsvar;
}