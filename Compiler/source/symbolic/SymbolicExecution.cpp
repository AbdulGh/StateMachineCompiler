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

    while (true)
    {
        while (theirIterator == currentCopyFrom->currentStack.rend())
        {
            if (!currentCopyFrom->getParent()) return change;
            currentCopyFrom = currentCopyFrom->getParent().get();
            theirIterator = currentCopyFrom->currentStack.rbegin();
        }

        if (myIterator == pseudoStack.rend())
        {
            vector<unique_ptr<SymVarStackMember>> newSt;
            while (!pseudoStack.empty())
            {
                newSt.push_back(move(pseudoStack.back()));
                pseudoStack.pop_back();
            }

            while (true)
            {
                while (theirIterator == currentCopyFrom->currentStack.rend())
                {
                    if (currentCopyFrom->getParent() == nullptr)
                    {
                        pseudoStack = move(newSt);
                        reverse(pseudoStack.begin(), pseudoStack.end());
                        return change;
                    }
                    else
                    {
                        currentCopyFrom = currentCopyFrom->getParent().get();
                        theirIterator = currentCopyFrom->currentStack.rbegin();
                    }
                }

                if ((*theirIterator)->getType() == SymbolicStackMemberType::STATE)
                {
                    if (returnStates.insert((*theirIterator)->getName()).second) change = true;
                    pseudoStack = move(newSt);
                    reverse(pseudoStack.begin(), pseudoStack.end());
                    return change;
                }
                newSt.push_back(move(static_cast<SymVarStackMember*>(theirIterator->get())->cloneVarMember()));
                change = true;
                ++theirIterator;
            }
            
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
    for (const string& n : visitOrder) retMe.emplace_back(Condition(pathConditions.at(n)));
    return retMe;
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
        auto& debug1 = it->first;
        auto& debug2 = it->second;
        bool optimising = true;
        if (!it->second->isLastNode() && visitedNodes.find(it->first) == visitedNodes.end()) //no feasable visits - remove
        {
            string s =  "State '" + it->first + "' is unreachable";
            if (optimising)
            {
                s += " and will be removed.";
                CFGNode *lonelyNode = it->second.get();
                if (lonelyNode->getCompSuccess() != nullptr) lonelyNode->getCompSuccess()->removeParent(lonelyNode);
                if (lonelyNode->getCompFail() != nullptr) lonelyNode->getCompFail()->removeParent(lonelyNode);
                for (auto& parentPair : lonelyNode->getPredecessorMap())
                {
                    CFGNode *parent = parentPair.second;
                    if (parent->getCompFail() != nullptr && parent->getCompFail()->getName() == lonelyNode->getName())
                    {
                        parent->setCompFail(parent->getCompSuccess());
                    } else if (!parent->isLastNode() &&
                               !(parent->getCompSuccess() != nullptr
                                 && parent->getCompSuccess()->getName() == lonelyNode->getName()))
                    {
                        throw runtime_error("bad parent");
                    }
                    parent->setCompSuccess(nullptr);
                    parent->setComp(nullptr);
                }
                lonelyNode->prepareToDie();
                it = cfg.removeNode(it->first);
            }
            else
            {
                s += ".";
                ++it;
            }
            reporter.optimising(Reporter::DEADCODE, s);
        }
        else ++it;
    }
    return tags;
}

bool SymbolicExecutionFringe::addPathCondition(const std::string& nodeName, JumpOnComparisonCommand* jocc, bool negate)
{
    if (hasSeen(nodeName)) throw std::runtime_error("cant visit node twice");
    else if (jocc->term1.getType() != StringType::ID) throw std::runtime_error("lhs should be ID");
    visitOrder.push_back(nodeName);
    Relations::Relop op = negate ? Relations::negateRelop(jocc->op) : jocc->op;
    pathConditions.insert({nodeName, Condition(jocc->term1, op, jocc->term2)});
    auto t1var = jocc->term1.getVarWrapper()->getSymbolicDouble(this);
    if (!t1var) throw std::runtime_error("comparing unknown var or constants");
    bool t1constructed = jocc->term1.getVarWrapper()->getSymbolicDouble(this).constructed();
    if (jocc->term2.isHolding())
    {
        auto t2var = jocc->term2.getVarWrapper()->getSymbolicDouble(this);
        if (!t2var) throw std::runtime_error("comparing unknown var");
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
                throw std::runtime_error("unknown op");
        }
    }
    else
    {
        short direction = 0;
        switch(op)
        {
            case Relations::LT:
                direction = -1;
            case Relations::LE:
                t1var->setRepeatUpperBound(jocc->term2.getLiteral(), direction);
                return t1var->clipUpperBound(jocc->term2.getLiteral(), direction);
            case Relations::GT:
                direction = false;
            case Relations::GE:
                t1var->setRepeatLowerBound(jocc->term2.getLiteral(), direction);
                return t1var->clipLowerBound(jocc->term2.getLiteral(), direction);
            case Relations::EQ:
                t1var->setConstValue(jocc->term2.getLiteral());
                return true;
            case Relations::NEQ: //todo neq consts
                return true;
            default:
                throw std::runtime_error("unknown relop");
        }
    }
}

CFGNode* SymbolicExecutionManager::getFailNode(shared_ptr<SymbolicExecutionFringe> returningSEF, CFGNode* n)
{
    CFGNode* failNode = n->getCompFail();
    if (failNode == nullptr) //return to top of stack
    {
        if (returningSEF->symbolicStack->isEmpty())
        {
            if (!n->isLastNode()) throw std::runtime_error("returns too early");
            return nullptr;
        }

        if (returningSEF->symbolicStack->peekTopType() != SymbolicStackMemberType::STATE) throw std::runtime_error("tried to jump to non-state");

        failNode = n->getParentGraph().getNode(returningSEF->symbolicStack->popState());
        if (failNode == nullptr) throw std::runtime_error("tried to jump to a nonexisting state");
        else //check its in successors of n
        if (find(n->getSuccessorVector().begin(), n->getSuccessorVector().end(), failNode) == n->getSuccessorVector().end())
        {
            throw std::runtime_error("should be successor");
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

    for (const auto& command : n->getInstrs()) if (!command->acceptSymbolicExecution(sef, true)) return;

    auto debug1 = osef->symbolicVarSet->findVar("LHS");
    auto debug2 = osef->symbolicVarSet->findVar("RHS");

    JumpOnComparisonCommand* jocc = n->getComp();
    if (jocc != nullptr) //is a conditional jump
    {
        //const comparisons were caught during compilation
        if (jocc->term1.getType() != StringType::ID) throw std::runtime_error("fail");

        //note: JOCC constructor ensures that if there is a var there is a var on the LHS
        auto LHS = jocc->term1.getVarWrapper()->getSymbolicDouble(sef.get());

        if (!LHS)
        {
            sef->error(Reporter::UNDECLARED_USE, "'" + jocc->term1.getVarWrapper()->getFullName() + "' used without being declared",
                       jocc->getLineNum());
            return;
        }

        if (!LHS->isDefined()) sef->warn(Reporter::UNINITIALISED_USE,
                                         "'" + LHS->getName() + "' used before being defined", jocc->getLineNum());

        //check if we can meet the comparison - search if so
        if (!jocc->term2.isHolding()) //comparing to a literal
        {
            double RHS = jocc->term2.getLiteral();

            switch (LHS->canMeet(jocc->op, RHS))
            {
                case SymbolicDouble::CANT:
                {
                    CFGNode* nextNode = getFailNode(sef, n);
                    if (nextNode != nullptr)
                    {
                        LHS->setRepeatBoundsFromComparison(Relations::negateRelop(jocc->op), RHS);
                        visitNode(sef, nextNode);
                    }
                    return;
                }
                case SymbolicDouble::MAY:
                {
                    branch(sef, n, jocc->term1.getVarWrapper(), jocc->op, RHS);
                    return;
                }
                case SymbolicDouble::MUST:
                {
                    LHS->setRepeatBoundsFromComparison(jocc->op, RHS);
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
            auto RHS = jocc->term2.getVarWrapper()->getSymbolicDouble(sef.get());
            if (!RHS)
            {
                sef->error(Reporter::UNDECLARED_USE, "'" + jocc->term2.getVarWrapper()->getFullName() + "' used without being declared",
                           jocc->getLineNum());
                return;
            }

            if (LHS->isDetermined())
            {
                if (RHS->isDetermined())
                {
                    if (Relations::evaluateRelop<double>(LHS->getConstValue(), jocc->op, RHS->getConstValue()))
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
                    branch(sef, n, jocc->term2.getVarWrapper(), mirroredOp, LHS->getConstValue(), true);
                    return;
                }
            }
            else if (RHS->isDetermined())
            {
                branch(sef, n, jocc->term1.getVarWrapper(), jocc->op, RHS->getConstValue(), false);
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
void SymbolicExecutionManager::branch(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n, VarWrapper* lhsvar,
                                      Relations::Relop op, double rhsconst, bool reverse)
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
                                        VarWrapper* lhsvar, double rhsconst, bool reverse)
{
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicDouble> gvpLT = lhsvar->getSymbolicDouble(seflt.get());
    if (gvpLT->setUpperBound(rhsconst, -1))
    {
        gvpLT->setRepeatUpperBound(rhsconst, -1);
        if (gvpLT.constructed()) lhsvar->setSymbolicDouble(seflt.get(), gvpLT.get());
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
    }

    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicDouble> gvpEQ = lhsvar->getSymbolicDouble(sefeq.get());
    gvpEQ->setConstValue(rhsconst);
    gvpEQ->setRepeatUpperBound(rhsconst);
    gvpEQ->setRepeatLowerBound(rhsconst);
    if (gvpEQ.constructed()) lhsvar->setSymbolicDouble(sefeq.get(), gvpEQ.get());
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
    GottenVarPtr<SymbolicDouble> gvpGT = lhsvar->getSymbolicDouble(seflt.get());
    if (gvpGT->setLowerBound(rhsconst, 1))
    {
        gvpGT->setRepeatLowerBound(rhsconst, 1);
        if (gvpGT.constructed()) lhsvar->setSymbolicDouble(sefgt.get(), gvpGT.get());

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
}


void SymbolicExecutionManager::branchNE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        VarWrapper* lhsvar, double rhsconst, bool reverse)
{
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicDouble> gvpLT = lhsvar->getSymbolicDouble(seflt.get());
    if (gvpLT->setUpperBound(rhsconst, -1))
    {
        gvpLT->setRepeatUpperBound(rhsconst, -1);
        if (gvpLT.constructed()) lhsvar->setSymbolicDouble(seflt.get(), gvpLT.get());
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
    }

    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicDouble> gvpEQ = lhsvar->getSymbolicDouble(sefeq.get());
    gvpEQ->setConstValue(rhsconst);
    gvpEQ->setRepeatUpperBound(rhsconst);
    gvpEQ->setRepeatLowerBound(rhsconst);
    if (gvpEQ.constructed()) lhsvar->setSymbolicDouble(sefeq.get(), gvpEQ.get());
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
    GottenVarPtr<SymbolicDouble> gvpGT = lhsvar->getSymbolicDouble(seflt.get());
    if (gvpGT->setLowerBound(rhsconst, 1))
    {
        gvpGT->setRepeatLowerBound(rhsconst, 1);
        if (gvpGT.constructed()) lhsvar->setSymbolicDouble(sefgt.get(), gvpGT.get());
        if (reverse)
        {
            if (!(n->isLastNode() && sef->symbolicStack->isEmpty())) {
                CFGNode* failNode = getFailNode(seflt, n);
                if (failNode != nullptr) visitNode(seflt, failNode);
            }
        }
        else visitNode(seflt, n->getCompSuccess());
    }
}

void SymbolicExecutionManager::branchLT(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        VarWrapper* lhsvar, double rhsconst, bool reverse)
{
    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicDouble> gvpLT = lhsvar->getSymbolicDouble(seflt.get());
    if (gvpLT->setUpperBound(rhsconst, -1))
    {
        gvpLT->setRepeatUpperBound(rhsconst, -1);
        if (gvpLT.constructed()) lhsvar->setSymbolicDouble(seflt.get(), gvpLT.get());

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
    }

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicDouble> gvpGE = lhsvar->getSymbolicDouble(sefge.get());
    if (gvpGE->setLowerBound(rhsconst))
    {
        gvpGE->setRepeatLowerBound(rhsconst);
        if (gvpGE.constructed()) lhsvar->setSymbolicDouble(sefge.get(), gvpGE.get());
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
}


void SymbolicExecutionManager::branchLE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        VarWrapper* lhsvar, double rhsconst, bool reverse)
{
    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicDouble> gvpLE = lhsvar->getSymbolicDouble(sefle.get());
    if (gvpLE->setUpperBound(rhsconst))
    {
        gvpLE->setRepeatUpperBound(rhsconst);
        if (gvpLE.constructed()) lhsvar->setSymbolicDouble(sefle.get(), gvpLE.get());

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
    }

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicDouble> gvpGT = lhsvar->getSymbolicDouble(sefgt.get());
    if (gvpGT->setLowerBound(rhsconst, 1))
    {
        gvpGT->setRepeatLowerBound(rhsconst, 1);
        if (gvpGT.constructed()) lhsvar->setSymbolicDouble(sefgt.get(), gvpGT.get());
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
}


void SymbolicExecutionManager::branchGT(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        VarWrapper* lhsvar, double rhsconst, bool reverse)
{
    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicDouble> gvpGT = lhsvar->getSymbolicDouble(sefgt.get());
    if (gvpGT->setLowerBound(rhsconst, 1))
    {
        gvpGT->setRepeatLowerBound(rhsconst, 1);
        if (gvpGT.constructed()) lhsvar->setSymbolicDouble(sefgt.get(), gvpGT.get());
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
    }

    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicDouble> gvpLE = lhsvar->getSymbolicDouble(sefle.get());
    if (gvpLE->setUpperBound(rhsconst))
    {
        gvpLE->setRepeatUpperBound(rhsconst);
        if (gvpLE.constructed()) lhsvar->setSymbolicDouble(sefle.get(), gvpLE.get());
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
}

void SymbolicExecutionManager::branchGE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                        VarWrapper* lhsvar, double rhsconst, bool reverse)
{
    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicDouble> gvpGE = lhsvar->getSymbolicDouble(sefge.get());
    if (gvpGE->setLowerBound(rhsconst))
    {
        if (gvpGE.constructed()) lhsvar->setSymbolicDouble(sefge.get(), gvpGE.get());

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
    }

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicDouble> gvpLT = lhsvar->getSymbolicDouble(seflt.get());
    if (gvpLT->setUpperBound(rhsconst, -1))
    {
        gvpLT->setRepeatUpperBound(rhsconst, -1);
        if (gvpLT.constructed()) lhsvar->setSymbolicDouble(seflt.get(), gvpLT.get());
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
}

//branching on var comparison
void SymbolicExecutionManager::varBranch(shared_ptr<SymbolicExecutionFringe>& sef, CFGNode* n,
                                         VarWrapper* LHS, Relations::Relop op, VarWrapper* RHS)
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
                                           VarWrapper* lhsvar, VarWrapper* rhsvar)
{
    if (!(n->isLastNode() && sef->symbolicStack->isEmpty())) 
    {
        shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
        CFGNode* failNode = getFailNode(seflt, n);
        if (failNode == nullptr) return;
        else
        {
            GottenVarPtr<SymbolicDouble> lhgv = lhsvar->getSymbolicDouble(seflt.get());
            if (lhgv->addLT(rhsvar, seflt.get(), lhgv.constructed())) visitNode(seflt, failNode);
        }
    }

    shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicDouble> lhgv = lhsvar->getSymbolicDouble(sefge.get());
    if (lhgv->addGE(rhsvar, sefge.get(), lhgv.constructed())) visitNode(sefge, n->getCompSuccess());
}

void SymbolicExecutionManager::varBranchGT(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                              VarWrapper* lhsvar, VarWrapper* rhsvar)
{
    if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
    {
        shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
        CFGNode* failNode = getFailNode(sefle, n);

        GottenVarPtr<SymbolicDouble> lhgv = lhsvar->getSymbolicDouble(sefle.get());
        if (lhgv->addLE(rhsvar, sefle.get(), lhgv.constructed())) visitNode(sefle, failNode);
    }

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicDouble> lhgv = lhsvar->getSymbolicDouble(sefgt.get());
    if (lhgv->addGE(rhsvar, sefgt.get(), lhgv.constructed())) visitNode(sefgt, n->getCompSuccess());
}

void SymbolicExecutionManager::varBranchLT(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                              VarWrapper* lhsvar, VarWrapper* rhsvar)
{
    if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
    {
        shared_ptr<SymbolicExecutionFringe> sefge = make_shared<SymbolicExecutionFringe>(sef);
        CFGNode* failNode = getFailNode(sefge, n);
        GottenVarPtr<SymbolicDouble> lhgv = lhsvar->getSymbolicDouble(sefge.get());
        if (lhgv->addGE(rhsvar, sefge.get(), lhgv.constructed())) visitNode(sefge, failNode);
    }

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicDouble> lhgv = lhsvar->getSymbolicDouble(seflt.get());
    if (lhgv->addLT(rhsvar, seflt.get(), lhgv.constructed())) visitNode(seflt, n->getCompSuccess());
}


void SymbolicExecutionManager::varBranchLE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                              VarWrapper* lhsvar, VarWrapper* rhsvar)
{
    if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
    {
        shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
        CFGNode* failNode = getFailNode(sefgt, n);
        GottenVarPtr<SymbolicDouble> lhgv = lhsvar->getSymbolicDouble(sefgt.get());
        if (lhgv->addGT(rhsvar, sefgt.get(), lhgv.constructed())) visitNode(sefgt, failNode);
    }

    shared_ptr<SymbolicExecutionFringe> sefle = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicDouble> lhgv = lhsvar->getSymbolicDouble(sefle.get());
    if (lhgv->addLE(rhsvar, sefle.get(), lhgv.constructed())) visitNode(sefle, n->getCompSuccess());
}


void SymbolicExecutionManager::varBranchNE(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                           VarWrapper* lhsvar, VarWrapper* rhsvar)
{
    if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
    {
        shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
        CFGNode* failNode = getFailNode(sefeq, n);
        if (failNode == nullptr) return;
        GottenVarPtr<SymbolicDouble> lhgv = lhsvar->getSymbolicDouble(sefeq.get());
        if (lhgv->addEQ(rhsvar, sefeq.get(), lhgv.constructed())) visitNode(sefeq, failNode);
    }

    shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicDouble> lhgv = lhsvar->getSymbolicDouble(sefgt.get());
    if (lhgv->addGE(rhsvar, sefgt.get(), lhgv.constructed())) visitNode(sefgt, n->getCompSuccess());
    sefgt.reset();

    shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
    GottenVarPtr<SymbolicDouble> lhgv2 = lhsvar->getSymbolicDouble(seflt.get());
    if (lhgv2->addGE(rhsvar, seflt.get(), lhgv2.constructed())) visitNode(seflt, n->getCompSuccess());
}


void SymbolicExecutionManager::varBranchEQ(shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                                           VarWrapper* lhsvar, VarWrapper* rhsvar)
{
    shared_ptr<SymbolicExecutionFringe> sefeq = make_shared<SymbolicExecutionFringe>(sef);
    CFGNode* failNode = getFailNode(sefeq, n);
    if (failNode == nullptr) return;
    GottenVarPtr<SymbolicDouble> lhgv = lhsvar->getSymbolicDouble(sefeq.get());
    if (lhgv->addEQ(rhsvar, sefeq.get(), lhgv.constructed())) visitNode(sefeq, n->getCompSuccess());


    if (!(n->isLastNode() && sef->symbolicStack->isEmpty()))
    {
        shared_ptr<SymbolicExecutionFringe> sefgt = make_shared<SymbolicExecutionFringe>(sef);
        CFGNode* failNode = getFailNode(sefgt, n);
        if (failNode == nullptr) return;
        GottenVarPtr<SymbolicDouble> lhgv = lhsvar->getSymbolicDouble(sefgt.get());
        if (lhgv->addGT(rhsvar, sefgt.get(), lhgv.constructed())) visitNode(sefgt, failNode);
        sefgt.reset();

        shared_ptr<SymbolicExecutionFringe> seflt = make_shared<SymbolicExecutionFringe>(sef);
        failNode = getFailNode(seflt, n);
        if (failNode == nullptr) return;
        GottenVarPtr<SymbolicDouble> lhgv2 = lhsvar->getSymbolicDouble(seflt.get());
        if (lhgv2->addLT(rhsvar, seflt.get(), lhgv2.constructed())) visitNode(seflt, failNode);
    }
}