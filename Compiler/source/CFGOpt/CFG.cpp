#include <algorithm>
#include <map>
#include <set>
#include <stack>
#include <memory>
#include <iostream>

#include "../compile/Functions.h"
#include "CFG.h"

using namespace std;

ControlFlowGraph::~ControlFlowGraph()
{
    for (auto& node : currentNodes) delete node.second;
}

CFGNode* ControlFlowGraph::getNode(const string& name)
{
    unordered_map<string, CFGNode*>::const_iterator it = currentNodes.find(name);
    if (it == currentNodes.cend()) return nullptr;
    return it->second;
}

void ControlFlowGraph::removeNode(string name)
{
    auto it = currentNodes.find(name);
    if (it == currentNodes.end()) throw "Check";
    CFGNode* nodePointer = it->second;
    if (nodePointer->isLastNode())
    {
        if (nodePointer->getPredecessors().size() != 1) throw runtime_error("Can't replace last node");
        CFGNode* newLast = last->getPredecessors().cbegin()->second;
        nodePointer->getParentFunction()->setLastNode(newLast);
        if (last->getName() == nodePointer->getName()) last = newLast;
    }
    delete nodePointer;
    currentNodes.erase(it);
}

CFGNode* ControlFlowGraph::createNode(const string &name, bool overwrite, bool isLast, FunctionSymbol* parentFunc)
{
    CFGNode* introducing;
    if ((introducing = getNode(name)) != nullptr)
    {
        if (!overwrite) throw runtime_error("node already exists");
        if (isLast) parentFunc->setLastNode(introducing);
        introducing->setLast(isLast);
    }
    else
    {
        introducing = new CFGNode(*this, parentFunc, name);
        if (isLast)
        {
            parentFunc->setLastNode(introducing);
            introducing->setLast(true);
        }
        currentNodes[introducing->getName()] = introducing;
    }
    return introducing;
}

CFGNode* ControlFlowGraph::createNode(const string &name, bool overwrite, bool isLast)
{
    CFGNode* introducing;
    if ((introducing = getNode(name)) != nullptr)
    {
        if (!overwrite) throw runtime_error("node already exists");
        introducing->setLast(isLast);
    }
    else
    {
        FunctionSymbol* parentFunc = functionTable.getParentFunc(name);
        CFGNode* nodePointer = new CFGNode(*this, parentFunc, name, isLast);
        introducing = nodePointer;
        currentNodes[introducing->getName()] = nodePointer;
    }

    return introducing;
}


void ControlFlowGraph::printSource()
{
    if (first == nullptr) return;
    first->printSource();
    cout << '\n';

    //order nodes
    //map<string, shared_ptr<CFGNode>> ordered(currentNodes.begin(), currentNodes.end());

    for (auto& it : currentNodes)
    {
        if (it.first == first->getName()) continue;
        it.second->printSource();
        cout << '\n';
    }
}

void ControlFlowGraph::printDotGraph()
{
    cout << "digraph{\n";
    if (first == nullptr)
    {
        cout << "}";
        return;
    }
    else first->printDotNode();

    map<string, CFGNode*> ordered(currentNodes.begin(), currentNodes.end());
    string currentFunc;
    for (auto& it : ordered) //the nodes should be grouped by function due to name ordering
    {
        if (it.first == first->getName()) continue;
        CFGNode* n = it.second;
        if (n->getParentFunction()->getPrefix() != currentFunc)
        {
            if (!currentFunc.empty()) cout << "}\n";
            FunctionSymbol* oldFS = functionTable.getFunction(currentFunc); //could be done by keeping a pointer but oh well
            string oldLastNode = oldFS->getLastNode()->getName();
            for (auto& nodePointer : oldFS->getReturnSuccessors())
            {
                cout << oldLastNode << "->" << nodePointer->getName()
                     << "[label='return'];\n";
            }
            currentFunc = n->getParentFunction()->getPrefix();
            cout << "subgraph cluster_" << currentFunc << "{\n";
            cout << "label='" << currentFunc << "';\n";
        }
        n->printDotNode();
        cout << '\n';
    }
}

unordered_map<string, CFGNode*>& ControlFlowGraph::getCurrentNodes()
{
    return currentNodes;
}

CFGNode* ControlFlowGraph::getFirst() const
{
    return first;
}

CFGNode* ControlFlowGraph::getLast() const
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