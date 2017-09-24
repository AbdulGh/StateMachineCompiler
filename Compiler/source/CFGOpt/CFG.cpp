#include <algorithm>
#include <map>
#include <set>
#include <stack>
#include <memory>

#include "../compile/FunctionCodeGen.h"
#include "CFG.h"

using namespace std;

/*nodes*/
const string &CFGNode::getName() const
{
    return name;
}

string CFGNode::getSource()
{
    stringstream outs;

    outs << name << '\n';
    for (shared_ptr<AbstractCommand>& ac: instrs) outs << ac->translation();
    if (compSuccess != nullptr) outs << comp->translation();
    if (compFail != nullptr) outs << JumpCommand(compFail->getName(), jumpline).translation();
    else outs << ReturnCommand(jumpline).translation();
    outs << "end" << '\n';
    return outs.str();
}

bool CFGNode::constProp()
{
    unordered_map<string,string> assignments;
    stack<vector<shared_ptr<AbstractCommand>>::iterator> pushedThings;

    auto it = instrs.begin();
    vector<shared_ptr<AbstractCommand>> newInstrs;
    newInstrs.reserve(instrs.size()); //avoid reallocation to keep iterators in pushedThings valid

    while (it != instrs.end())
    {
        shared_ptr<AbstractCommand> current = move(*it);
        if (current->getType() == CommandType::CHANGEVAR)
        {
            assignments.erase(current->getData());
            newInstrs.push_back(current);
        }
        else if (current->getType() == CommandType::ASSIGNVAR)
        {
            shared_ptr<AssignVarCommand> avc = static_pointer_cast<AssignVarCommand>(current);
            if (AbstractCommand::getStringType(avc->RHS) != AbstractCommand::StringType::ID)
            {
                assignments[avc->getData()] = avc->RHS;
            }
            else
            {
                unordered_map<string, string>::const_iterator constit = assignments.find(avc->RHS);
                if (constit != assignments.end())
                {
                    assignments[avc->getData()] = constit->second;
                    avc->RHS = constit->second;
                }
                else
                {
                    assignments[avc->getData()] = avc->RHS;
                }
            }
            newInstrs.push_back(current);
        }
        else if (current->getType() == CommandType::EXPR)
        {
            shared_ptr<EvaluateExprCommand> eec = static_pointer_cast<EvaluateExprCommand>(current);
            //literals will not be found
            unordered_map<string, string>::const_iterator t1it = assignments.find(eec->term1);
            unordered_map<string, string>::const_iterator t2it = assignments.find(eec->term2);
            if (t1it != assignments.end()) eec->term1 = t1it->second;
            if (t2it != assignments.end()) eec->term2 = t2it->second;
            if (AbstractCommand::getStringType(eec->term1) != AbstractCommand::StringType::ID
                && AbstractCommand::getStringType(eec->term2) != AbstractCommand::StringType::ID)
            {
                try
                {
                    double lhs = stod(t1it->second);
                    double rhs = stod(t2it->second);
                    double result = evaluateOp(lhs, eec->op, rhs);
                    string resultstr = to_string(result);
                    assignments[eec->getData()] = resultstr;
                    newInstrs.push_back(make_shared<AssignVarCommand>(eec->getData(), resultstr, eec->getLineNum()));
                }
                catch (invalid_argument& e)
                {
                    if (eec->op == PLUS)
                    {
                        string result = eec->term1 + eec->term2;
                        assignments[eec->getData()] = result;
                        newInstrs.push_back(make_shared<AssignVarCommand>(eec->getData(), result, eec->getLineNum()));
                    }
                    else throw runtime_error("Strings only support +");
                }
            }
            else
            {
                assignments.erase(eec->getData());
                newInstrs.push_back(current);
            }
        }
        else if (current->getType() == CommandType::PUSH)
        {
            shared_ptr<PushCommand> pushc = static_pointer_cast<PushCommand>(current);
            if (pushc->pushType == PushCommand::PUSHVAR)
            {
                auto pushedVarIt = assignments.find(current->getData());
                if (pushedVarIt != assignments.end()) current->setData(pushedVarIt->second);
            }
            newInstrs.push_back(current);
            pushedThings.push(prev(newInstrs.end()));
        }
        else if (current->getType() == CommandType::POP && !pushedThings.empty())
        {
            shared_ptr<PopCommand> popc = static_pointer_cast<PopCommand>(current);
            auto stackTop = pushedThings.top();
            shared_ptr<PushCommand> pushc = static_pointer_cast<PushCommand>(*stackTop);
            if (pushc->pushType == PushCommand::PUSHSTATE) throw runtime_error("tried to pop state into var");
            else
            {
                pushedThings.pop();
                newInstrs.push_back(make_shared<AssignVarCommand>(current->getData(), pushc->getData(), current->getLineNum()));
                newInstrs.erase(stackTop);
            }
        }
        else newInstrs.push_back(current);
        it++;
    }

    bool skippedReturn = false;
    if (comp != nullptr)
    {
        if (comp->term1Type == AbstractCommand::StringType::ID &&
            assignments.find(comp->term1) != assignments.end()) comp->setTerm1(assignments[comp->term1]);
        if (comp->term2Type == AbstractCommand::StringType::ID &&
            assignments.find(comp->term2) != assignments.end()) comp->setTerm2(assignments[comp->term2]);
        comp->makeGood();
    }
    /* todo make this work
    else if (compFail == nullptr && !pushedThings.empty()) //there should be a state on top
    {
        auto stackTop = pushedThings.top();
        shared_ptr<PushCommand> pushc = static_pointer_cast<PushCommand>(*stackTop);
        if (pushc->pushType != PushCommand::PUSHSTATE) throw runtime_error("tried to jump to var");
        std::string debug = pushc->getData();
        shared_ptr<CFGNode> jumpingTo = parentGraph.getNode(pushc->getData());
        if (jumpingTo == nullptr) throw runtime_error("Tried to jump to nonexistent state");
        compFail = jumpingTo;
        newInstrs.erase(stackTop);
        if (!returnTo.size() == 1) throw "check";
        returnTo.at(0)->removeParent(name);
        returnTo.clear();
        skippedReturn = true;
    }*/
    instrs = move(newInstrs);
    return skippedReturn;
}

bool CFGNode::addParent(shared_ptr<CFGNode> parent)
{
    auto it = predecessors.find(parent->getName());
    if (it != predecessors.end()) return false;
    predecessors[parent->getName()] = parent;
    return true;
}

void CFGNode::removeParent(shared_ptr<CFGNode> leaving)
{
    removeParent(leaving->getName());
}

void CFGNode::removeParent(const string& s)
{
    if (predecessors.erase(s) == 0) throw runtime_error("Parent is not in");
}

void CFGNode::setInstructions(vector<shared_ptr<AbstractCommand>>& in)
{
    instrs.clear();
    auto it = in.begin();

    while (it != in.end()
           && (*it)->getType() != CommandType::JUMP
           && (*it)->getType() != CommandType::CONDJUMP)
    {
        shared_ptr<AbstractCommand> current = move(*it);
        instrs.push_back(current);
        ++it;
    }

    if (it == in.cend())
    {
        return;
    }

    if ((*it)->getType() == CommandType::CONDJUMP)
    {
        compSuccess = parentGraph.getNode((*it)->getData());
        if (compSuccess == nullptr) compSuccess = parentGraph.createNode((*it)->getData(), nullptr, false, false); //will be created properly later
        compSuccess->addParent(shared_from_this());
        comp = static_pointer_cast<JumpOnComparisonCommand>(move(*it)->clone());

        if (++it == in.cend()) return;
    }

    if ((*it)->getType() == CommandType::JUMP)
    {
        string jumpto = (*it)->getData();
        jumpline = (*it)->getLineNum();
        if (jumpto == "return") compFail = nullptr;
        else
        {
            compFail = parentGraph.getNode(jumpto);
            if (compFail == nullptr) compFail = parentGraph.createNode(jumpto, nullptr, false, false);
            compFail->addParent(shared_from_this());
        }
        if (++it != in.cend()) throw "Should end here";
    }
    else throw "Can only end w/ <=2 jumps";
}

shared_ptr<JumpOnComparisonCommand> CFGNode::getComp()
{
    return comp;
}

unordered_map<string, shared_ptr<CFGNode>>& CFGNode::getPredecessors()
{
    return predecessors;
}

shared_ptr<CFGNode> CFGNode::getCompSuccess()
{
    return compSuccess;
}

vector<shared_ptr<CFGNode>>& CFGNode::getReturnSuccessors()
{
    return returnTo;
}

shared_ptr<CFGNode> CFGNode::getCompFail()
{
    return compFail;
}

vector<shared_ptr<AbstractCommand>> &CFGNode::getInstrs()
{
    return instrs;
}

void CFGNode::setCompSuccess(const shared_ptr<CFGNode> &compSuccess)
{
    CFGNode::compSuccess = compSuccess;
}

void CFGNode::setCompFail(const shared_ptr<CFGNode> &compFail)
{
    CFGNode::compFail = compFail;
}

void CFGNode::setComp(const shared_ptr<JumpOnComparisonCommand> &comp)
{
    CFGNode::comp = comp;
}

bool CFGNode::swallowNode(shared_ptr<CFGNode> other)
{
    if (compSuccess == nullptr)
    {
        if (compFail != nullptr &&  compFail->getName() == other->getName()
            || returnTo.size() == 1 && returnTo.at(0)->getName() == other->getName())
        {
            vector<shared_ptr<AbstractCommand>> newInstrs = instrs;
            vector<shared_ptr<AbstractCommand>>& addingInstrs = other->getInstrs();
            newInstrs.reserve(instrs.size() + addingInstrs.size() + 2);
            for (shared_ptr<AbstractCommand>& newInst : addingInstrs) newInstrs.push_back(newInst->clone());
            setInstructions(newInstrs);
            if (other->getComp() != nullptr)
            {
                setComp(static_pointer_cast<JumpOnComparisonCommand>(other->getComp()->clone()));
            }
            else setComp(nullptr);
            setCompSuccess(other->getCompSuccess());
            setCompFail(other->getCompFail());
            if (getCompSuccess() != nullptr) getCompSuccess()->addParent(shared_from_this());
            if (getCompFail() != nullptr) getCompFail()->addParent(shared_from_this());
            else
            {
                returnTo.reserve(returnTo.size() + other->returnTo.size());
                for (const shared_ptr<CFGNode>& retNode : other->returnTo)
                {
                    returnTo.push_back(retNode);
                    retNode->addParent(shared_from_this());
                }
            }
            return true;
        }
    }
    return false;
}

ControlFlowGraph& CFGNode::getParentGraph() const
{
    return parentGraph;
}

FunctionCodeGen* CFGNode::getParentFunction() const
{
    return parentFunction;
}

int CFGNode::getJumpline() const
{
    return jumpline;
}

bool CFGNode::isLastNode() const
{
    return isLast;
}

void CFGNode::addReturnSuccessor(shared_ptr<CFGNode> returningTo)
{
    if (name == "F_main_fin")
    {
        int debug;
        debug = 2;
    }
    returnTo.push_back(returningTo);
}

/*graph*/

shared_ptr<CFGNode> ControlFlowGraph::getNode(const string& name)
{
    unordered_map<string, shared_ptr<CFGNode>>::const_iterator it = currentNodes.find(name);
    if (it == currentNodes.cend()) return nullptr;
    return it->second;
}

void ControlFlowGraph::removeNode(string name)
{
    if (last != nullptr && last->getName() == name)
    {
        if (last->getPredecessors().size() != 1) throw runtime_error("Can't replace last node");
        last = last->getPredecessors().cbegin()->second;
    }
    auto it = currentNodes.find(name);
    if (it == currentNodes.end()) throw "Check";
    shared_ptr<CFGNode> nodePointer = it->second;
    if (nodePointer->isLastNode())
    {
        if (nodePointer->getPredecessors().size() != 1) throw runtime_error("Can't replace last node");
        shared_ptr<CFGNode> newLast = last->getPredecessors().cbegin()->second;
        nodePointer->getParentFunction()->setLastNode(newLast);
        if (last->getName() == nodePointer->getName()) last = newLast;
    }
    currentNodes.erase(it);
}

shared_ptr<CFGNode>
ControlFlowGraph::createNode(const string &name, FunctionCodeGen* parentFunc, bool overwrite, bool isLast)
{
    shared_ptr<CFGNode> introducing;
    if ((introducing = getNode(name)) != nullptr)
    {
        if (!overwrite) throw runtime_error("node already exists");
    }
    else
    {
        introducing = make_shared<CFGNode>(*this, parentFunc, name, isLast);
        currentNodes[introducing->getName()] = introducing;
    }
    return introducing;
}

string ControlFlowGraph::getSource()
{
    if (first == nullptr) return "";
    stringstream outs;
    outs << first->getSource() << '\n';

    for (auto it : currentNodes)
    {
        if (it.first == first->getName()) continue;
        outs << it.second->getSource() << '\n';
    }

    return outs.str();
}

unordered_map<string, shared_ptr<CFGNode>> &ControlFlowGraph::getCurrentNodes()
{
    return currentNodes;
}

shared_ptr<CFGNode> ControlFlowGraph::getFirst() const
{
    return first;
}

shared_ptr<CFGNode> ControlFlowGraph::getLast() const
{
    return last;
}

void ControlFlowGraph::setFirst(const string& firstName)
{
    auto it = currentNodes.find(firstName);
    if (it == currentNodes.end()) throw "Check";
    first = it->second;
}

void ControlFlowGraph::setLast(const string& lastName)
{
    auto it = currentNodes.find(lastName);
    if (it == currentNodes.end()) throw "Check";
    last = it->second;
}