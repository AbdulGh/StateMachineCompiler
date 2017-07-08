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
    if (compFail != nullptr) outs << JumpCommand(compFail->getName()).translation();
    outs << "end" << endl;
    return outs.str();
}

void CFGNode::addParent(std::shared_ptr<CFGNode> parent)
{
    predecessors.push_back(parent); //todo consider the possibility of duplication
}

void CFGNode::removeParent(std::shared_ptr<CFGNode> leaving)
{
    auto it = find(predecessors.begin(), predecessors.end(), leaving);

    if (it != predecessors.end())
    {
        swap(*it, predecessors.back());
        predecessors.pop_back();
    }
}

void CFGNode::setInstructions(const vector<std::shared_ptr<AbstractCommand>> &in)
{
    vector<std::shared_ptr<AbstractCommand>>::const_iterator it = in.cbegin();

    while (it != in.cend() &&
            (*it)->getEffect() != CommandSideEffect::JUMP && (*it)->getEffect() != CommandSideEffect::CONDJUMP)
    {
        instrs.push_back(*it);
        it++;
    }

    if (it == in.cend())
    {
        return;
    }

    if ((*it)->getEffect() == CommandSideEffect::CONDJUMP) //todo consider pop
    {
        comp = static_pointer_cast<JumpOnComparisonCommand>(*it);
        compSuccess = parent.createNodeIfNotExists(comp->getData());
        compSuccess->addParent(shared_from_this());

        if (++it == in.cend()) return;
    }

    if ((*it)->getEffect() == CommandSideEffect::JUMP)
    {
        compFail = parent.createNodeIfNotExists((*it)->getData());
        compFail->addParent(shared_from_this());
        if (++it != in.cend()) throw "Should end here";
    }
    else throw "Can only end w/ <=2 jumps";
}

shared_ptr<JumpOnComparisonCommand> &CFGNode::getComp()
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

/*graph*/

shared_ptr<CFGNode> ControlFlowGraph::createNodeIfNotExists(string name) //todo consider returning nullptr for pop
{
    unordered_map<string, shared_ptr<CFGNode>>::const_iterator it = currentNodes.find(name);
    if (it == currentNodes.cend())
    {
        shared_ptr<CFGNode> p(new CFGNode(*this, name));
        currentNodes[name] = p;
        return p;
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
    shared_ptr<CFGNode> introducing = createNodeIfNotExists(name);
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
        if (it->first == first->getName() || it->first == "pop") continue;
        outs << it->second->getSource() << endl;
    }

    return outs.str();
}

unordered_map<string, shared_ptr<CFGNode>> &ControlFlowGraph::getCurrentNodes()
{
    return currentNodes;
}
