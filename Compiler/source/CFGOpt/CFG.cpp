#include <algorithm>
#include <map>

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
    std::stringstream outs;

    outs << name << endl;
    for (shared_ptr<AbstractCommand> ac: instrs) outs << ac->translation();
    if (compSuccess != nullptr) outs << comp->translation();
    if (compFail != nullptr) outs << JumpCommand(compFail->getName(), jumpline).translation();
    else outs << ReturnCommand(jumpline).translation();
    outs << "end" << endl;
    return outs.str();
}

void CFGNode::addParent(std::shared_ptr<CFGNode> parent)
{
    auto it = find(predecessors.begin(), predecessors.end(), parent);
    if (it != predecessors.end()) throw "Parent already in";
    predecessors.push_back(parent);
}

void CFGNode::removeParent(std::shared_ptr<CFGNode> leaving)
{
    auto it = find(predecessors.begin(), predecessors.end(), leaving);

    if (it != predecessors.end())
    {
        swap(*it, predecessors.back());
        predecessors.pop_back();
    }
    else  throw "Parent is not in";
}

void CFGNode::setInstructions(const vector<std::shared_ptr<AbstractCommand>> &in)
{
    vector<std::shared_ptr<AbstractCommand>>::const_iterator it = in.cbegin();

    while (it != in.cend()
           &&  (*it)->getEffectFlag() != CommandSideEffect::JUMP
           && (*it)->getEffectFlag() != CommandSideEffect::CONDJUMP)
    {
        instrs.push_back(*it);
        it++;
    }

    if (it == in.cend())
    {
        return;
    }

    if ((*it)->getEffectFlag() == CommandSideEffect::CONDJUMP)
    {
        comp = static_pointer_cast<JumpOnComparisonCommand>(*it);
        compSuccess = parent.getNode((*it)->getData());
        compSuccess->addParent(shared_from_this());

        if (++it == in.cend()) return;
    }

    if ((*it)->getEffectFlag() == CommandSideEffect::JUMP)
    {
        string jumpto = (*it)->getData();
        jumpline = (*it)->getLineNum();
        if (jumpto == "return") compFail = nullptr;
        else
        {
            compFail = parent.getNode(jumpto);
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

vector<shared_ptr<CFGNode>> &CFGNode::getPredecessors()
{
    return predecessors;
}

shared_ptr<CFGNode> CFGNode::getCompSuccess()
{
    return compSuccess;
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

ControlFlowGraph &CFGNode::getParent() const
{
    return parent;
}

int CFGNode::getJumpline() const
{
    return jumpline;
}

/*graph*/

shared_ptr<CFGNode> ControlFlowGraph::getNode(string name, bool create)
{
    unordered_map<string, shared_ptr<CFGNode>>::const_iterator it = currentNodes.find(name);
    if (it == currentNodes.cend())
    {
        if (create)
        {
            shared_ptr<CFGNode> p = make_shared<CFGNode>(*this, name);
            currentNodes[name] = p;
            return p;
        }
        else return nullptr;
    }
    return it->second;
}

void ControlFlowGraph::removeNode(std::string name)
{
    unordered_map<string, shared_ptr<CFGNode>>::iterator it = currentNodes.find(name);
    if (it == currentNodes.end()) throw "Check";
    currentNodes.erase(it);
}

void ControlFlowGraph::addNode(std::string name, std::vector<std::shared_ptr<AbstractCommand>> instrs)
{
    shared_ptr<CFGNode> introducing = getNode(name);
    if (currentNodes.size() == 1) first = introducing;
    introducing->setInstructions(instrs);
}

std::string ControlFlowGraph::getSource()
{
    if (first == nullptr) return "";
    std::stringstream outs;
    outs << first->getSource() << endl;

    for (auto it = currentNodes.cbegin(); it != currentNodes.cend(); it++)
    {
        if (it->first == first->getName()) continue;
        outs << it->second->getSource() << endl;
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

void ControlFlowGraph::setLast(std::string lastName)
{
    unordered_map<string, shared_ptr<CFGNode>>::iterator it = currentNodes.find(lastName);
    if (it == currentNodes.end()) throw "Check";
    last = it->second;
}