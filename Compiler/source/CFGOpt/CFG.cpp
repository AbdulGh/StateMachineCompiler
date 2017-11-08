#include <algorithm>
#include <map>
#include <set>
#include <stack>
#include <memory>
#include <iostream>

#include "../compile/Functions.h"
#include "CFG.h"

using namespace std;

CFGNode* ControlFlowGraph::getNode(const string& name)
{
    unordered_map<string, unique_ptr<CFGNode>>::const_iterator it = currentNodes.find(name);
    if (it == currentNodes.cend()) return nullptr;
    return it->second.get();
}

void ControlFlowGraph::removeNode(string name)
{
    auto it = currentNodes.find(name);
    if (it == currentNodes.end()) throw "Check";
    unique_ptr<CFGNode>& nodePointer = it->second;
    if (nodePointer->isLastNode())
    {
        if (nodePointer->getPredecessorMap().size() != 1) throw runtime_error("Can't replace last node");
        CFGNode* newLast = last->getPredecessorMap().cbegin()->second;
        nodePointer->getParentFunction()->setLastNode(newLast);
        if (last->getName() == nodePointer->getName()) last = newLast;
    }
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
        unique_ptr<CFGNode> newPtr = make_unique<CFGNode>(*this, parentFunc, name);
        introducing = newPtr.get();
        if (isLast)
        {
            parentFunc->setLastNode(introducing);
            introducing->setLast(true);
        }
        currentNodes[introducing->getName()] = move(newPtr);
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
        unique_ptr<CFGNode> nodePointer = make_unique<CFGNode>(*this, parentFunc, name, isLast);
        introducing = nodePointer.get();
        currentNodes[introducing->getName()] = move(nodePointer);
    }

    return introducing;
}

string ControlFlowGraph::getStructuredSource()
{
    if (first == nullptr) return "";

    stringstream outs;
    outs << first->getSource();
    outs << '\n';

    //order nodes
    map<string, CFGNode*> ordered;
    for (auto& node : currentNodes) ordered[node.first] = node.second.get();

    for (auto& it : ordered)
    {
        if (it.first == first->getName()) continue;
        outs << it.second->getSource() << '\n';
    }

    return outs.str();
}

string ControlFlowGraph::destroyStructureAndGetFinalSource() //todo redo this
{
    bool changes = true;
    while (changes)
    {
        changes = false;
        unordered_map<string, unique_ptr<CFGNode>>& nodes = getCurrentNodes();
        auto it = nodes.begin();
        while (it != nodes.end())
        {
            CFGNode* current = it->second.get();
            if (current->getInstrs().empty() && current->getCompSuccess() == nullptr)
            {
                if (current->getCompFail() == nullptr) current->removePushes();
                else current->replacePushes(current->getCompFail()->getName());


                if (current->getName() == "F0_main_13")
                {
                    auto debug2 = current->getPredecessorVector();
                    int debug;
                    debug = 2;
                }

                for (auto& parent : current->getPredecessorVector())
                {
                    if ((parent->getCompSuccess() == nullptr || parent->getCompSuccess()->getName() != current->getName())
                        && parent->getCompFail() == nullptr && parent->isLastNode())
                    {
                        parent->getParentFunction()->removeReturnSuccessor(current->getName());
                    }
                    else if (!parent->swallowNode(current)) throw "should swallow";
                }
                if (current->getName() == first->getName())
                {
                    if (current->getCompFail() != nullptr) first = getNode(current->getCompFail()->getName());
                    else if (current->getParentFunction()->getReturnSuccessors().size() == 1)
                    {
                        first = *(current->getParentFunction()->getReturnSuccessors().cbegin());
                    }
                    else if (nodes.size() != 1) throw "should have been one of those";
                }

                it = nodes.erase(it);
                changes=true;
            }
            else ++it;
        }
    }
    return getStructuredSource();
}

string ControlFlowGraph::getDotGraph()
{
    stringstream outs;
    outs << "digraph{\ngraph [pad=\"0.8\", nodesep=\"0.8\", ranksep=\"0.8\"];\n";
    if (first == nullptr)
    {
        outs << "}";
        return outs.str();
    }
    map<string, CFGNode*> ordered;
    for (auto& node : currentNodes) ordered[node.first] = node.second.get();
    
    string currentFunc;
    for (auto& it : ordered) //the nodes should be grouped by function due to name ordering
    {
        CFGNode* n = it.second;
        if (n->getParentFunction()->getIdent() != currentFunc)
        {
            if (!currentFunc.empty())
            {
                outs << "}\n";
                FunctionSymbol* oldFS = functionTable.getFunction(currentFunc); //could be done by keeping a pointer but oh well
                string oldLastNode = oldFS->getLastNode()->getName();
                for (auto& nodePointer : oldFS->getReturnSuccessors())
                {
                    outs << oldLastNode << "->" << nodePointer->getName()
                         << "[label=\"return\"];\n";
                }
            }
            currentFunc = n->getParentFunction()->getIdent();
            outs << "subgraph cluster_" << currentFunc << "{\n";
            outs << "label=\"" << currentFunc << "\";\n";
        }
        outs << n->getDotNode();
        outs << '\n';
    }
    outs << "}}\n";
    return outs.str();
}

unordered_map<string, unique_ptr<CFGNode>>& ControlFlowGraph::getCurrentNodes()
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

Reporter& ControlFlowGraph::getReporter() const
{
    return reporter;
}

void ControlFlowGraph::setFirst(const string& firstName)
{
    auto it = currentNodes.find(firstName);
    if (it == currentNodes.end()) throw "Check";
    first = it->second.get();
}

void ControlFlowGraph::setLast(const string& lastName)
{
    auto it = currentNodes.find(lastName);
    if (it == currentNodes.end()) throw "Check";
    last = it->second.get();
}