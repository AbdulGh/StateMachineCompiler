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

bool SymbolicExecutionFringe::hasSeen(const string& state)
{
    if (pathConditions.find(state) != pathConditions.end()) return true;
    else return (parent != nullptr && checkParentPC && parent->hasSeen(state));
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

bool SymbolicExecutionFringe::addPathCondition(const std::string& nodeName, JumpOnComparisonCommand* jocc, bool negate)
{
    if (hasSeen(nodeName)) throw "cant visit node twice";
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
                return t1var->addLT(t2var);
            case Relations::LE:
                return t1var->addLE(t2var);
            case Relations::GT:
                return t1var->addGT(t2var);
            case Relations::GE:
                return t1var->addGE(t2var);
            case Relations::EQ:
                return t1var->addEQ(t2var);
            case Relations::NEQ:
                return t1var->addNEQ(t2var);
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
                return t1var->clipUpperBound(jocc->term2, closed);
            case Relations::GT:
                closed = true;
            case Relations::GE:
                return t1var->clipLowerBound(jocc->term2, closed);
            case Relations::EQ:
                t1var->setConstValue(jocc->term2);
                return true;
            case Relations::NEQ:
                t1var->addNEQConst(jocc->term2);
                return true;
            default:
                throw "unknown relop";
        }
    }
}

SymbolicVariable* SymbolicExecutionManager::SearchResult::nextPop()
{
    if (currentPop >= poppedVars.size()) throw "went too far";
    ++currentPop;
    return poppedVars[currentPop-1]->clone().release(); //todo less roundabout
}

void SymbolicExecutionManager::SearchResult::addPop(SymbolicVariable* popped)
{
    if (currentPop == poppedVars.size()) poppedVars.push_back(popped->clone().release());
    else
    {
        SymbolicVariable* sv = poppedVars[currentPop];
        sv->unionLowerBound(popped->getLowerBound());
        sv->unionUpperBound(popped->getUpperBound());
    }
    ++currentPop;
}

//SymbolicExecutionManager
unordered_map<string, unique_ptr<SymbolicExecutionManager::SearchResult>>& SymbolicExecutionManager::search()
{
    visitedNodes.clear();
    tags.clear();
    for (auto& pair : cfg.getCurrentNodes()) tags[pair.first] = make_unique<SearchResult>();
    shared_ptr<SymbolicExecutionFringe> sef = make_shared<SymbolicExecutionFringe>(reporter);
    visitNode(sef, cfg.getFirst());
    auto it = cfg.getCurrentNodes().begin();
    while (it != cfg.getCurrentNodes().end())
    {
        if (it->second->isLastNode())
        {
            ++it;
            continue;
        }
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
                else if (!parent->isLastNode()) throw runtime_error("bad parent"); //done in removePushes
                parent->setComp(nullptr);
            }
            lonelyNode->removePushes();
            it = cfg.removeNode(it->first);
        }
        else ++it;
    }
    return tags;
}

CFGNode* SymbolicExecutionManager::getFailNode(shared_ptr<SymbolicExecutionFringe> returningSEF, CFGNode* n)
{
    CFGNode* failNode = n->getCompFail();
    if (failNode == nullptr) //return to top of stack
    {
        if (returningSEF->currentStack->isEmpty())
        {
            if (!n->isLastNode()) throw "returns too early";
            return nullptr;
        }
        if (returningSEF->currentStack->getTopType() != SymbolicStackMemberType::STATE) throw "tried to jump to non-state";
        failNode = n->getParentGraph().getNode(returningSEF->currentStack->popState());
        if (failNode == nullptr) throw "tried to jump to a nonexisting state";
        else //check its in successors of n
        if (find(n->getSuccessorVector().begin(), n->getSuccessorVector().end(), failNode) == n->getSuccessorVector().end())
        {
            throw "should be successor";
        }
    }
    return failNode;
}

//todo seen function calls
bool SymbolicExecutionManager::visitNode(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n)
{
    if (!sef->isFeasable()) return false;
    else if (sef->hasSeen(n->getName())) return true;

    unique_ptr<SearchResult>& thisNodeSR = tags[n->getName()];
    thisNodeSR->svs.unionSVS(sef->symbolicVarSet.get());

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
        else
        {
            if (command->getType() == CommandType::POP)
            {
                if (sef->currentStack->isEmpty() || sef->currentStack->getTopType() != VAR)
                {
                    sef->error(Reporter::BAD_STACK_USE,
                               "Tried to pop empty stack, or non var into var", command->getLineNum());

                }
                SymbolicVariable* poppedVar = sef->currentStack->peekTopVar();
                thisNodeSR->addPop(poppedVar);
            }
            if (!command->acceptSymbolicExecution(sef)) return false;
        }
    }

    if (n->callsFunction()) //search mutual recursion
    {
        sef->pathConditions.clear();
        sef->checkParentPC = false;
    }

    //don't track conditions till later - this just tracks which nodes we've seen
    sef->pathConditions.insert({n->getName(), Condition()});
    visitedNodes.insert(n->getName());

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
        CFGNode* retNode = getFailNode(sef, n);
        if (retNode == nullptr) return n->isLastNode(); //return on an empty stack means we exit
        else return visitNode(sef, retNode);
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
        case Relations::NEQ:
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
        if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
        else
        {
            CFGNode* failNode = getFailNode(seflt, n);
            if (failNode == nullptr) return false;
            visitWithFeasable(seflt, failNode);
        }
    }
    seflt.reset();

    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    sefeq->symbolicVarSet->findVar(lhsvar)->setConstValue(rhsconst);
    if (reverse)
    {
        if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
        else
        {
            CFGNode *failNode = getFailNode(sefeq, n);
            if (failNode == nullptr) return false;
            visitWithFeasable(sefeq, failNode);
        }
    }
    else visitWithFeasable(sefeq, n->getCompSuccess());

    sefeq.reset();

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst, true);

    if (reverse) {visitWithFeasable(sefgt, n->getCompSuccess());}
    else
    {
        if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
        else
        {
            CFGNode *failNode = getFailNode(sefgt, n);
            if (failNode == nullptr) return false;
            visitWithFeasable(sefgt, failNode);
        }
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
        if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
        else
        {
            CFGNode *failNode = getFailNode(seflt, n);
            if (failNode == nullptr) return false;
            visitWithFeasable(seflt, failNode);
        }
    }
    else visitWithFeasable(seflt, n->getCompSuccess());
    seflt.reset();
    
    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    sefeq->symbolicVarSet->findVar(lhsvar)->setConstValue(rhsconst);
    if (reverse) {visitWithFeasable(seflt, n->getCompSuccess());}
    else
    {
        if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
        else
        {
            CFGNode *failNode = getFailNode(sefeq, n);
            if (failNode == nullptr) return false;
            visitWithFeasable(sefeq, failNode);
        }
    }
    sefeq.reset();
    

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst, true);
    if (reverse)
    {
        if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
        else
        {
            CFGNode *failNode = getFailNode(seflt, n);
            if (failNode == nullptr) return false;
            visitWithFeasable(seflt, failNode);
        }
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
        if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
        else
        {
            CFGNode *failNode = getFailNode(seflt, n);
            if (failNode == nullptr) return false;
            visitWithFeasable(seflt, failNode);
        }
    }
    else visitWithFeasable(seflt, n->getCompSuccess());
    seflt.reset();
    
    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    sefge->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst);
    if (reverse) {visitWithFeasable(sefge, n->getCompSuccess());}
    else
    {
        if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
        else
        {
            CFGNode *failNode = getFailNode(sefge, n);
            if (failNode == nullptr) return false;
            visitWithFeasable(sefge, failNode);
        }
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
        if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
        else
        {
            CFGNode *failNode = getFailNode(sefle, n);
            if (failNode == nullptr) return false;
            visitWithFeasable(sefle, failNode);
        }
    }
    else visitWithFeasable(sefle, n->getCompSuccess());
    sefle.reset();

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst, true);
    if (reverse) {visitWithFeasable(sefgt, n->getCompSuccess());}
    else
    {
        if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
        else
        {
            CFGNode *failNode = getFailNode(sefgt, n);
            if (failNode == nullptr) return false;
            visitWithFeasable(sefgt, failNode);
        }
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
        if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
        else
        {
            CFGNode *failNode = getFailNode(sefgt, n);
            if (failNode == nullptr) return false;
            visitWithFeasable(sefgt, failNode);
        }
    }
    else visitWithFeasable(sefgt, n->getCompSuccess());
    sefgt.reset();

    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    sefle->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst);

    if (reverse) {visitWithFeasable(sefle, n->getCompSuccess());}
    else
    {
        if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
        else
        {
            CFGNode *failNode = getFailNode(sefle, n);
            if (failNode == nullptr) return false;
            visitWithFeasable(sefle, failNode);
        }
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
        if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
        else
        {
            CFGNode *failNode = getFailNode(sefge, n);
            if (failNode == nullptr) return false;
            visitWithFeasable(sefge, failNode);
        }
    }
    else visitWithFeasable(sefge, n->getCompSuccess());
    sefge.reset();
    

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst, true);
    if (reverse) {visitWithFeasable(seflt, n->getCompSuccess());}
    else
    {
        if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
        else
        {
            CFGNode *failNode = getFailNode(seflt, n);
            if (failNode == nullptr) return false;
            visitWithFeasable(seflt, failNode);
        }
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
        case Relations::NEQ:
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

    if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
    else
    {
        shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
        CFGNode *failNode = getFailNode(seflt, n);
        if (failNode == nullptr) return false;
        else
        {
            SymbolicVariable* newLHSVar = seflt->symbolicVarSet->findVar(lhsvar->getName());
            SymbolicVariable* newRHSVar = seflt->symbolicVarSet->findVar(rhsvar->getName());
            if (newLHSVar->addLT(newRHSVar)) visitWithFeasable(seflt, failNode);
        }
    }

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    SymbolicVariable* newLHSVar = sefge->symbolicVarSet->findVar(lhsvar->getName());
    SymbolicVariable* newRHSVar = sefge->symbolicVarSet->findVar(rhsvar->getName());
    if (newLHSVar->addGE(newRHSVar)) visitWithFeasable(sefge, n->getCompSuccess());
    return feasable;
}


bool SymbolicExecutionManager::varBranchGT(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                              SymbolicVariable* lhsvar, SymbolicVariable* rhsvar)
{
    bool feasable = false;
    if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
    else
    {
        shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
        CFGNode *failNode = getFailNode(sefle, n);
        SymbolicVariable* newLHSVar = sefle->symbolicVarSet->findVar(lhsvar->getName());
        SymbolicVariable* newRHSVar = sefle->symbolicVarSet->findVar(rhsvar->getName());
        if (newLHSVar->addLE(newRHSVar)) visitWithFeasable(sefle, failNode);
    }

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    SymbolicVariable* newLHSVar = sefgt->symbolicVarSet->findVar(lhsvar->getName());
    SymbolicVariable* newRHSVar = sefgt->symbolicVarSet->findVar(rhsvar->getName());
    if (newLHSVar->addGT(newRHSVar)) visitWithFeasable(sefgt, n->getCompSuccess());
    return feasable;
}

bool SymbolicExecutionManager::varBranchLT(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                              SymbolicVariable* lhsvar, SymbolicVariable* rhsvar)
{
    bool feasable = false;

    if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
    else
    {
        shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
        CFGNode *failNode = getFailNode(sefge, n);
        SymbolicVariable* newLHSVar = sefge->symbolicVarSet->findVar(lhsvar->getName());
        SymbolicVariable* newRHSVar = sefge->symbolicVarSet->findVar(rhsvar->getName());
        if (newLHSVar->addGE(newRHSVar)) visitWithFeasable(sefge, failNode);
    }

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    SymbolicVariable* newLHSVar = seflt->symbolicVarSet->findVar(lhsvar->getName());
    SymbolicVariable* newRHSVar = seflt->symbolicVarSet->findVar(rhsvar->getName());
    if (newLHSVar->addLT(newRHSVar)) visitWithFeasable(seflt, n->getCompSuccess());
    return feasable;
}


bool SymbolicExecutionManager::varBranchLE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                              SymbolicVariable* lhsvar, SymbolicVariable* rhsvar)
{
    bool feasable = false;

    if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
    else
    {
        shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
        CFGNode *failNode = getFailNode(sefgt, n);
        SymbolicVariable *newLHSVar = sefgt->symbolicVarSet->findVar(lhsvar->getName());
        SymbolicVariable *newRHSVar = sefgt->symbolicVarSet->findVar(rhsvar->getName());
        if (newLHSVar->addGT(newRHSVar)) visitWithFeasable(sefgt, failNode);
    }

    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    SymbolicVariable* newLHSVar = sefle->symbolicVarSet->findVar(lhsvar->getName());
    SymbolicVariable* newRHSVar = sefle->symbolicVarSet->findVar(rhsvar->getName());
    if (newLHSVar->addLE(newRHSVar)) visitWithFeasable(sefle, n->getCompSuccess());
    return feasable;
}


bool SymbolicExecutionManager::varBranchNE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                           SymbolicVariable* lhsvar, SymbolicVariable* rhsvar)
{
    bool feasable = false;

    if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
    else
    {
        shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
        CFGNode *failNode = getFailNode(sefeq, n);
        if (failNode == nullptr) return false;
        SymbolicVariable *newLHSvar = sefeq->symbolicVarSet->findVar(lhsvar->getName());
        SymbolicVariable *newRHSvar = sefeq->symbolicVarSet->findVar(rhsvar->getName());

        if (newLHSvar->addEQ(newRHSvar)) visitWithFeasable(sefeq, failNode);
    }

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    SymbolicVariable *newLHSvar = sefgt->symbolicVarSet->findVar(lhsvar->getName());
    SymbolicVariable *newRHSvar = sefgt->symbolicVarSet->findVar(rhsvar->getName());
    if (newLHSvar->addGT(newRHSvar)) visitWithFeasable(sefgt, n->getCompSuccess());
    sefgt.reset();

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    newLHSvar = seflt->symbolicVarSet->findVar(lhsvar->getName());
    newRHSvar = seflt->symbolicVarSet->findVar(rhsvar->getName());
    if (newLHSvar->addLT(newRHSvar)) visitWithFeasable(seflt, n->getCompSuccess());
    return feasable;
}


bool SymbolicExecutionManager::varBranchEQ(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                           SymbolicVariable* lhsvar, SymbolicVariable* rhsvar)
{
    bool feasable = false;

    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    CFGNode *failNode = getFailNode(sefeq, n);
    if (failNode == nullptr) return false;
    SymbolicVariable *newLHSvar = sefeq->symbolicVarSet->findVar(lhsvar->getName());
    SymbolicVariable *newRHSvar = sefeq->symbolicVarSet->findVar(rhsvar->getName());
    if (newLHSvar->addEQ(newRHSvar)) visitWithFeasable(sefeq, failNode);


    if (n->isLastNode() && sef->currentStack->isEmpty()) feasable = true;
    else
    {
        shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
        CFGNode* failNode = getFailNode(sefgt, n);
        if (failNode == nullptr) return false;
        SymbolicVariable *newLHSvar = sefgt->symbolicVarSet->findVar(lhsvar->getName());
        SymbolicVariable *newRHSvar = sefgt->symbolicVarSet->findVar(rhsvar->getName());
        if (newLHSvar->addGT(newRHSvar)) visitWithFeasable(sefgt, failNode);
        sefgt.reset();

        shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
        failNode = getFailNode(seflt, n);
        if (failNode == nullptr) return false;
        newLHSvar = seflt->symbolicVarSet->findVar(lhsvar->getName());
        newRHSvar = seflt->symbolicVarSet->findVar(rhsvar->getName());
        if (newLHSvar->addLT(newRHSvar)) visitWithFeasable(seflt, failNode);
    }
    return feasable;
}