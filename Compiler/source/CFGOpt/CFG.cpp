#include <algorithm>
#include <map>
#include <set>
#include <stack>
#include <memory>

#include "../compile/FunctionCodeGen.h"
#include "CFG.h"

using namespace std;

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
        introducing->setParentFunction(parentFunc);
        introducing->setLast(isLast);
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
    first->putSource(outs);
    outs << '\n';

    //order nodes
    map<string, shared_ptr<CFGNode>> ordered(currentNodes.begin(), currentNodes.end());

    for (auto it : ordered)
    {
        if (it.first == first->getName()) continue;
        it.second->putSource(outs);
        outs<< '\n';
    }

    return outs.str();
}

string ControlFlowGraph::getDotGraph()
{
    stringstream outs;
    outs << "digraph{\n";
    if (first == nullptr)
    {
        outs << "}";
        return outs.str();
    }
    else first->putDotNode(outs);

    map<string, shared_ptr<CFGNode>> ordered(currentNodes.begin(), currentNodes.end());
    string currentFunc;
    for (auto it : ordered) //the nodes should be grouped by function due to name ordering
    {
        if (it.first == first->getName()) continue;
        shared_ptr<CFGNode> n = it.second;
        if (n->getParentFunction()->getIdentifier() != currentFunc)
        {
            if (!currentFunc.empty()) outs << "}\n";
            currentFunc = n->getParentFunction()->getIdentifier();
            outs << "subgraph cluster_" << currentFunc << "{\n";
            outs << "label='" << currentFunc << "';\n";
        }
        n->putDotNode(outs);
        outs<< '\n';
    }

    return outs.str();
}

unordered_map<string, shared_ptr<CFGNode>>& ControlFlowGraph::getCurrentNodes()
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