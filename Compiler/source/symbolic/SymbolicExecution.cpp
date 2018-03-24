#include <algorithm>

#include "SymbolicExecution.h"
#include "VarWrappers.h"

using namespace std;
using namespace SymbolicExecution;

//SearchResult
bool SymbolicExecutionManager::SearchResult::unionStack(SymbolicStack* other)
{
    bool change = false;
    auto myIterator = pseudoStack.rbegin();
    SymbolicStack* currentCopyFrom = other;
    while (currentCopyFrom->currentStack.empty() && currentCopyFrom->getParent() != nullptr)
    {
        currentCopyFrom = currentCopyFrom->getParent().get();
    }
    if (currentCopyFrom->currentStack.empty()) return false;
    auto theirIterator = currentCopyFrom->currentStack.rbegin();

    while (theirIterator != currentCopyFrom->currentStack.rend()
           && !(theirIterator == currentCopyFrom->currentStack.rend() && currentCopyFrom->getParent() != nullptr))
    {
        if (myIterator == pseudoStack.rend())
        {
            vector<unique_ptr<SymVarStackMember>> newSt;
            while (!pseudoStack.empty())
            {
                newSt.push_back(move(pseudoStack.back()));
                pseudoStack.pop_back();
            }

            for (auto tempIt = currentCopyFrom->currentStack.rbegin();;) //???
            {
                while (tempIt == currentCopyFrom->currentStack.rend())
                {
                    if (currentCopyFrom->getParent() == nullptr)
                    {
                        pseudoStack = move(newSt);
                        reverse(pseudoStack.begin(), pseudoStack.end());
                        return true;
                    }
                    else
                    {
                        currentCopyFrom = currentCopyFrom->getParent().get();
                        tempIt = currentCopyFrom->currentStack.rbegin();
                    }
                }

                if ((*tempIt)->getType() == SymbolicStackMemberType::STATE)
                {
                    returnStates.insert((*tempIt)->getName());
                    pseudoStack = move(newSt);
                    reverse(pseudoStack.begin(), pseudoStack.end());
                    return true;
                }
                newSt.push_back(move(static_cast<SymVarStackMember*>((*tempIt)->clone().get())->cloneVarMember()));
                ++tempIt;
            }
        }
        if (theirIterator == currentCopyFrom->currentStack.rend() && currentCopyFrom->getParent() != nullptr)
        {
            currentCopyFrom = currentCopyFrom->getParent().get();
            theirIterator = currentCopyFrom->currentStack.rbegin();
        }

        while (myIterator != pseudoStack.rend() && theirIterator != currentCopyFrom->currentStack.rend())
        {
            if ((*theirIterator)->getType() == SymbolicStackMemberType::STATE)
            {
                return returnStates.insert((*theirIterator)->getName()).second;
            }
            if ((*myIterator)->mergeSM(*theirIterator)) change = true;
            ++myIterator; ++theirIterator;
        }
    }
    return change;
}

//SymbolicExecutionFringe
SymbolicExecutionFringe::SymbolicExecutionFringe(Reporter &r) :
        parent{},
        reporter(r),
        symbolicStack(make_shared<SymbolicStack>(r)),
        symbolicVarSet(make_shared<SymbolicVarSet>(nullptr)){}

SymbolicExecutionFringe::SymbolicExecutionFringe(shared_ptr<SymbolicExecutionFringe> p):
        parent(p),
        reporter(p->reporter),
        symbolicStack(make_shared<SymbolicStack>(p->symbolicStack)),
        symbolicVarSet(make_shared<SymbolicVarSet>(p->symbolicVarSet))
        {seenFunctionCalls = p->seenFunctionCalls;}


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
    if (pathConditions.find(state) != pathConditions.end()
        || find(seenFunctionCalls.begin(), seenFunctionCalls.end(), state) != seenFunctionCalls.end()) return true;
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
    for (const string& nodeName : visitOrder)
    {
        out += "Visit " + nodeName + " - branch " + pathConditions.at(nodeName).toString() + "\n";
    }
    return out;
}

vector<Condition> SymbolicExecutionFringe::getConditions()
{
    vector<Condition> retMe;
    if (parent != nullptr) retMe = parent->getConditions();
    for (const string& n : visitOrder) retMe.push_back(pathConditions[n]);
    return retMe;
}

bool SymbolicExecutionFringe::addPathCondition(const std::string& nodeName, JumpOnComparisonCommand* jocc, bool negate)
{
    if (hasSeen(nodeName)) throw "cant visit node twice";
    else if (jocc->term1.getType() != StringType::ID) throw "lhs should be ID";
    visitOrder.push_back(nodeName);
    Relations::Relop op = negate ? Relations::negateRelop(jocc->op) : jocc->op;
    pathConditions.insert({nodeName, Condition(string(jocc->term1), op, string(jocc->term2))});
    auto t1var = jocc->term1.getVarWrapper()->getSymbolicVariable(this);
    if (!t1var) throw "comparing unknown var or constants";
    bool t1constructed = jocc->term1.getVarWrapper()->getSymbolicVariable(this).constructed(); //todo make this less embarassing
    if (jocc->term2.getType() == StringType::ID)
    {
        auto t2var = jocc->term2.getVarWrapper()->getSymbolicVariable(this);
        if (!t2var) throw "comparing unknown var";
        switch(op)
        {
            case Relations::LT:
                return t1var->addLT(jocc->term2.getVarWrapper(), this, t1constructed);
            case Relations::LE:
                return t1var->addLE(jocc->term2.getVarWrapper(), this, t1constructed);
            case Relations::GT:
                return t1var->addGT(jocc->term2.getVarWrapper(), this, t1constructed);
            case Relations::GE:
                return t1var->addGE(jocc->term2.getVarWrapper(), this, t1constructed);
            case Relations::EQ:
                return t1var->addEQ(jocc->term2.getVarWrapper(), this, t1constructed);
            case Relations::NEQ:
                return t1var->addNEQ(jocc->term2.getVarWrapper(), this, t1constructed);
            default:
                throw "unknown op";
        }
    }
    else
    {
        bool closed = true;
        switch(op)
        {
            case Relations::LT:
                closed = false;
            case Relations::LE:
                return t1var->clipUpperBound(string(jocc->term2), closed);
            case Relations::GT:
                closed = false;
            case Relations::GE:
                return t1var->clipLowerBound(string(jocc->term2), closed);
            case Relations::EQ:
                t1var->setConstValue(string(jocc->term2));
                return true;
            case Relations::NEQ:
                t1var->addNEQConst(string(jocc->term2));
                return true;
            default:
                throw "unknown relop";
        }
    }
}

void SymbolicExecutionFringe::setLoopInit()
{
    symbolicVarSet->setLoopInit();
    symbolicStack->setLoopInit();
}

//SymbolicExecutionManager
unordered_map<string, unique_ptr<SymbolicExecutionManager::SearchResult>>& SymbolicExecutionManager::search()
{
    visitedNodes.clear();
    tags.clear();
    for (auto& pair : cfg.getCurrentNodes()) tags[pair.first] = make_unique<SearchResult>(reporter);
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
        if (visitedNodes.find(it->first) == visitedNodes.end()) //no feasable visits - remove
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
                else if (!parent->isLastNode()) throw runtime_error("bad parent");
                parent->setComp(nullptr);
            }
            lonelyNode->prepareToDie();
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
        if (returningSEF->symbolicStack->isEmpty())
        {
            if (!n->isLastNode()) throw "returns too early";
            return nullptr;
        }

        if (returningSEF->symbolicStack->peekTopType() != SymbolicStackMemberType::STATE) throw "tried to jump to non-state";

        failNode = n->getParentGraph().getNode(returningSEF->symbolicStack->popState());
        if (failNode == nullptr) throw "tried to jump to a nonexisting state";
        else //check its in successors of n
        if (find(n->getSuccessorVector().begin(), n->getSuccessorVector().end(), failNode) == n->getSuccessorVector().end())
        {
            throw "should be successor";
        }
    }
    return failNode;
}

void SymbolicExecutionManager::visitNode(shared_ptr<SymbolicExecutionFringe> osef, CFGNode* n)
{
    if (!osef->isFeasable()) return;


    unique_ptr<SearchResult>& thisNodeSR = tags[n->getName()];
    bool change = thisNodeSR->unionSVS(osef->symbolicVarSet.get());
    if (thisNodeSR->unionStack(osef->symbolicStack.get())) change = true;
    if (!visitedNodes.insert(n->getName()).second && !change) return; //seen before

    shared_ptr<SymbolicExecutionFringe> sef = make_shared<SymbolicExecutionFringe>(osef);

    for (const auto& command : n->getInstrs())
    {
        if (command->getType() == CommandType::POP)
        {
            sef->symbolicStack->popVar();
            if (command->getVarWrapper()) command->getVarWrapper()->nondet(sef.get());
        }
        else if (!command->acceptSymbolicExecution(sef, true)) return;
    }

    JumpOnComparisonCommand* jocc = n->getComp();
    if (jocc != nullptr) //is a conditional jump
    {
        //const comparisons were caught during compilation
        if (jocc->term1.getType() != StringType::ID) throw "fail";

        //note: JOCC constructor ensures that if there is a var there is a var on the LHS
        auto LHS = jocc->term1.getVarWrapper()->getSymbolicVariable(sef.get());

        if (!LHS)
        {
            sef->error(Reporter::UNDECLARED_USE, "'" + jocc->term1.getVarWrapper()->getFullName() + "' used without being declared",
                       jocc->getLineNum());
            return;
        }

        if (!LHS->isDefined()) sef->warn(Reporter::UNINITIALISED_USE,
                                         "'" + LHS->getName() + "' used before being defined", jocc->getLineNum());

        //check if we can meet the comparison - search if so
        if (jocc->term2.getType() != StringType::ID) //comparing to a literal
        {
            if ((jocc->term2.getType() == StringType::DOUBLELIT && LHS->getType() != DOUBLE)
                    || (jocc->term2.getType() == StringType::STRINGLIT && LHS->getType() != STRING))
            {
                sef->error(Reporter::TYPE, "'" + jocc->term1.getVarWrapper()->getFullName() + "' (type " + TypeEnumNames[LHS->getType()]
                                           + ")  compared to a different type",
                           jocc->getLineNum());
                return;
            }

            const string& rhs = *jocc->term2.getString();

            if (LHS->isDetermined() && LHS->meetsConstComparison(jocc->op, rhs)) return visitNode(sef, n->getCompSuccess());

            switch (LHS->canMeet(jocc->op, rhs))
            {
                case SymbolicVariable::CANT:
                {
                    CFGNode* nextNode = getFailNode(sef, n);
                    if (nextNode != nullptr) visitNode(sef, nextNode);
                    return;
                }
                case SymbolicVariable::MAY:
                {
                    branch(sef, n, LHS->getName(), jocc->op, rhs);
                    return;
                }
                case SymbolicVariable::MUST:
                {
                    visitNode(sef, n->getCompSuccess());
                    return;
                }
                default:
                    throw runtime_error("very weird enum");
                return;
            }
        }
        else //comparing to another variable
        {
            auto RHS = jocc->term2.getVarWrapper()->getSymbolicVariable(sef.get());
            if (!RHS)
            {
                sef->error(Reporter::UNDECLARED_USE, "'" + jocc->term2.getVarWrapper()->getFullName() + "' used without being declared",
                           jocc->getLineNum());
                return;
            }

            if (LHS->getType() != RHS->getType())
            {
                sef->error(Reporter::TYPE, "'" + jocc->term1.getVarWrapper()->getFullName() + "' (type " + TypeEnumNames[LHS->getType()] +
                        ") compared to '" + jocc->term2.getVarWrapper()->getFullName() + "' (type " + TypeEnumNames[RHS->getType()] + ")",
                           jocc->getLineNum());
                return;
            }

            if (LHS->isDetermined())
            {
                if (RHS->isDetermined())
                {
                    if (LHS->meetsConstComparison(jocc->op, RHS->getConstString()))
                    {
                        visitNode(sef, n->getCompSuccess());
                        return;
                    }
                    else
                    {
                        CFGNode* nextnode = getFailNode(sef, n);
                        if (nextnode != nullptr) visitNode(sef, nextnode);
                        return;
                    }
                }
                else //rhs undetermined, lhs determined
                {
                    Relations::Relop mirroredOp = Relations::mirrorRelop(jocc->op);
                    branch(sef, n, RHS->getName(), mirroredOp, LHS->getConstString(), true);
                    return;
                }
            }
            else if (RHS->isDetermined())
            {
                branch(sef, n, LHS->getName(), jocc->op, RHS->getConstString(), false);
                return;
            }

            //neither are determined here
            varBranch(sef, n, jocc->term1.getVarWrapper(), jocc->op, jocc->term2.getVarWrapper());
        }
    }
    else //unconditional jump
    {
        CFGNode* retNode = getFailNode(sef, n);
        if (retNode != nullptr) visitNode(sef, retNode); //return on an empty stack means we exit
    }
}

//these things below will usually be called when we already have
//a ptr to the vars but we want to copy that var into the 'new scope'
void SymbolicExecutionManager::branch(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n, string lhsvar,
                                      Relations::Relop op, const std::string& rhsconst, bool reverse)
{
    switch(op)
    {
        case Relations::EQ:
            branchEQ(sef, n, lhsvar, rhsconst, reverse);
            break;
        case Relations::NEQ:
            branchNE(sef, n, lhsvar, rhsconst, reverse);
            break;
        case Relations::LT:
            branchLT(sef, n, lhsvar, rhsconst, reverse);
            break;
        case Relations::LE:
            branchLE(sef, n, lhsvar, rhsconst, reverse);
            break;
        case Relations::GT:
            branchGT(sef, n, lhsvar, rhsconst, reverse);
            break;
        case Relations::GE:
            branchGE(sef, n, lhsvar, rhsconst, reverse);
            break;
        default:
            throw runtime_error("bad relop");
    }
}

void SymbolicExecutionManager::branchEQ(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        string lhsvar, const std::string& rhsconst, bool reverse)
{
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst, false);
    if (reverse) {visitNode(seflt, n->getCompSuccess());}
    else
    {
        if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
        {
            CFGNode* failNode = getFailNode(seflt, n);
            if (failNode != nullptr) visitNode(seflt, failNode);
        }
    }
    seflt.reset();

    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    sefeq->symbolicVarSet->findVar(lhsvar)->setConstValue(rhsconst);
    if (reverse)
    {
        if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
        {
            CFGNode* failNode = getFailNode(sefeq, n);
            if (failNode != nullptr) visitNode(sefeq, failNode);
        }
    }
    else visitNode(sefeq, n->getCompSuccess());

    sefeq.reset();

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst, true);

    if (reverse) {visitNode(sefgt, n->getCompSuccess());}
    else
    {
        if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
        {
            CFGNode* failNode = getFailNode(sefgt, n);
            if (failNode != nullptr) visitNode(sefgt, failNode);
        }
    }
}


void SymbolicExecutionManager::branchNE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        string lhsvar, const std::string& rhsconst, bool reverse)
{
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst, false);
    if (reverse)
    {
        if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
        {
            CFGNode* failNode = getFailNode(seflt, n);
            if (failNode != nullptr) visitNode(seflt, failNode);
        }
    }
    else visitNode(seflt, n->getCompSuccess());
    seflt.reset();

    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    sefeq->symbolicVarSet->findVar(lhsvar)->setConstValue(rhsconst);
    if (reverse) {visitNode(seflt, n->getCompSuccess());}
    else
    {
        if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
        {
            CFGNode* failNode = getFailNode(sefeq, n);
            if (failNode != nullptr) visitNode(sefeq, failNode);
        }
    }
    sefeq.reset();


    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst, true);
    if (reverse)
    {
        if (!(n->isLastNode() && sef->symbolicStack->isEmpty())) {
            CFGNode* failNode = getFailNode(seflt, n);
            if (failNode != nullptr) visitNode(seflt, failNode);
        }
    }
    else visitNode(seflt, n->getCompSuccess());
}


void SymbolicExecutionManager::branchLT(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        string lhsvar, const std::string& rhsconst, bool reverse)
{
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst, false);

    if (reverse)
    {
        if (!(n->isLastNode() && sef->symbolicStack->isEmpty())) 
        {
            CFGNode* failNode = getFailNode(seflt, n);
            if (failNode != nullptr) visitNode(seflt, failNode);
        }
    }
    else visitNode(seflt, n->getCompSuccess());
    seflt.reset();

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    sefge->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst);
    if (reverse) {visitNode(sefge, n->getCompSuccess());}
    else
    {
        if (!(n->isLastNode() && sef->symbolicStack->isEmpty())) 
        {
            CFGNode* failNode = getFailNode(sefge, n);
            if (failNode != nullptr) visitNode(sefge, failNode);
        }
    }
}


void SymbolicExecutionManager::branchLE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        string lhsvar, const std::string& rhsconst, bool reverse)
{
    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    sefle->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst);

    if (reverse)
    {
        if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
        {
            CFGNode* failNode = getFailNode(sefle, n);
            if (failNode != nullptr) visitNode(sefle, failNode);
        }
    }
    else visitNode(sefle, n->getCompSuccess());
    sefle.reset();

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst, true);
    if (reverse) {visitNode(sefgt, n->getCompSuccess());}
    else
    {
        if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
        {
            CFGNode* failNode = getFailNode(sefgt, n);
            if (failNode != nullptr) visitNode(sefgt, failNode);
        }
    }
}


void SymbolicExecutionManager::branchGT(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        string lhsvar, const std::string& rhsconst, bool reverse)
{
    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    sefgt->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst, false);
    if (reverse)
    {
        if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
        {
            CFGNode* failNode = getFailNode(sefgt, n);
            if (failNode != nullptr) visitNode(sefgt, failNode);
        }
    }
    else visitNode(sefgt, n->getCompSuccess());
    sefgt.reset();

    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    sefle->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst);

    if (reverse) {visitNode(sefle, n->getCompSuccess());}
    else
    {
        if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
        {
            CFGNode* failNode = getFailNode(sefle, n);
            if (failNode != nullptr) visitNode(sefle, failNode);
        }
    }
}


void SymbolicExecutionManager::branchGE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        string lhsvar, const std::string& rhsconst, bool reverse)
{
    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    sefge->symbolicVarSet->findVar(lhsvar)->setLowerBound(rhsconst);

    if (reverse)
    {
        if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
        {
            CFGNode* failNode = getFailNode(sefge, n);
            if (failNode != nullptr) visitNode(sefge, failNode);
        }
    }
    else visitNode(sefge, n->getCompSuccess());
    sefge.reset();


    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    seflt->symbolicVarSet->findVar(lhsvar)->setUpperBound(rhsconst, true);
    if (reverse) {visitNode(seflt, n->getCompSuccess());}
    else
    {
        if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
        {
            CFGNode* failNode = getFailNode(seflt, n);
            if (failNode != nullptr) visitNode(seflt, failNode);
        }
    }
}

//branching on var comparison
void SymbolicExecutionManager::varBranch(shared_ptr<SymbolicExecutionFringe>& sef, CFGNode* n,
                                         const VarWrapper* LHS, Relations::Relop op, const VarWrapper* RHS)
{
    switch(op)
    {
        case Relations::EQ:
            varBranchEQ(sef, n, LHS, RHS);
            break;
        case Relations::NEQ:
            varBranchNE(sef, n, LHS, RHS);
            break;
        case Relations::LT:
            varBranchLT(sef, n, LHS, RHS);
            break;
        case Relations::LE:
            varBranchLE(sef, n, LHS, RHS);
            break;
        case Relations::GT:
            varBranchGT(sef, n, LHS, RHS);
            break;
        case Relations::GE:
            varBranchGE(sef, n, LHS, RHS);
            break;
        default:
            throw runtime_error("bad relop");
    }
}

void SymbolicExecutionManager::varBranchGE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                           const VarWrapper* lhsvar, const VarWrapper* rhsvar)
{
    if (!(n->isLastNode() && sef->symbolicStack->isEmpty())) 
    {
        shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
        CFGNode* failNode = getFailNode(seflt, n);
        if (failNode == nullptr) return;
        else
        {
            GottenVarPtr<SymbolicVariable> lhgv = lhsvar->getSymbolicVariable(seflt.get());
            if (lhgv->addLT(rhsvar, seflt.get(), lhgv.constructed())) visitNode(seflt, failNode);
        }
    }

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicVariable> lhgv = lhsvar->getSymbolicVariable(sefge.get());
    if (lhgv->addGE(rhsvar, sefge.get(), lhgv.constructed())) visitNode(sefge, n->getCompSuccess());
}


void SymbolicExecutionManager::varBranchGT(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                              const VarWrapper* lhsvar, const VarWrapper* rhsvar)
{
    if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
    {
        shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
        CFGNode* failNode = getFailNode(sefle, n);

        GottenVarPtr<SymbolicVariable> lhgv = lhsvar->getSymbolicVariable(sefle.get());
        if (lhgv->addLE(rhsvar, sefle.get(), lhgv.constructed())) visitNode(sefle, failNode);
    }

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicVariable> lhgv = lhsvar->getSymbolicVariable(sefgt.get());
    if (lhgv->addGE(rhsvar, sefgt.get(), lhgv.constructed())) visitNode(sefgt, n->getCompSuccess());
}

void SymbolicExecutionManager::varBranchLT(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                              const VarWrapper* lhsvar, const VarWrapper* rhsvar)
{
    if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
    {
        shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
        CFGNode* failNode = getFailNode(sefge, n);
        GottenVarPtr<SymbolicVariable> lhgv = lhsvar->getSymbolicVariable(sefge.get());
        if (lhgv->addGE(rhsvar, sefge.get(), lhgv.constructed())) visitNode(sefge, failNode);
    }

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicVariable> lhgv = lhsvar->getSymbolicVariable(seflt.get());
    if (lhgv->addGE(rhsvar, seflt.get(), lhgv.constructed())) visitNode(seflt, n->getCompSuccess());
}


void SymbolicExecutionManager::varBranchLE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                              const VarWrapper* lhsvar, const VarWrapper* rhsvar)
{
    if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
    {
        shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
        CFGNode* failNode = getFailNode(sefgt, n);
        GottenVarPtr<SymbolicVariable> lhgv = lhsvar->getSymbolicVariable(sefgt.get());
        if (lhgv->addGT(rhsvar, sefgt.get(), lhgv.constructed())) visitNode(sefgt, failNode);
    }

    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicVariable> lhgv = lhsvar->getSymbolicVariable(sefle.get());
    if (lhgv->addLE(rhsvar, sefle.get(), lhgv.constructed())) visitNode(sefle, n->getCompSuccess());
}


void SymbolicExecutionManager::varBranchNE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                           const VarWrapper* lhsvar, const VarWrapper* rhsvar)
{
    if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
    {
        shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
        CFGNode* failNode = getFailNode(sefeq, n);
        if (failNode == nullptr) return;
        GottenVarPtr<SymbolicVariable> lhgv = lhsvar->getSymbolicVariable(sefeq.get());
        if (lhgv->addEQ(rhsvar, sefeq.get(), lhgv.constructed())) visitNode(sefeq, failNode);
    }

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicVariable> lhgv = lhsvar->getSymbolicVariable(sefgt.get());
    if (lhgv->addGE(rhsvar, sefgt.get(), lhgv.constructed())) visitNode(sefgt, n->getCompSuccess());
    sefgt.reset();

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicVariable> lhgv2 = lhsvar->getSymbolicVariable(seflt.get());
    if (lhgv2->addGE(rhsvar, seflt.get(), lhgv2.constructed())) visitNode(seflt, n->getCompSuccess());
}


void SymbolicExecutionManager::varBranchEQ(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                           const VarWrapper* lhsvar, const VarWrapper* rhsvar)
{
    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    CFGNode* failNode = getFailNode(sefeq, n);
    if (failNode == nullptr) return;
    GottenVarPtr<SymbolicVariable> lhgv = lhsvar->getSymbolicVariable(sefeq.get());
    if (lhgv->addEQ(rhsvar, sefeq.get(), lhgv.constructed())) visitNode(sefeq, n->getCompSuccess());


    if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
    {
        shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
        CFGNode* failNode = getFailNode(sefgt, n);
        if (failNode == nullptr) return;
        GottenVarPtr<SymbolicVariable> lhgv = lhsvar->getSymbolicVariable(sefgt.get());
        if (lhgv->addGT(rhsvar, sefgt.get(), lhgv.constructed())) visitNode(sefgt, failNode);
        sefgt.reset();

        shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
        failNode = getFailNode(seflt, n);
        if (failNode == nullptr) return;
        GottenVarPtr<SymbolicVariable> lhgv2 = lhsvar->getSymbolicVariable(seflt.get());
        if (lhgv2->addLT(rhsvar, seflt.get(), lhgv2.constructed())) visitNode(seflt, failNode);
    }
}