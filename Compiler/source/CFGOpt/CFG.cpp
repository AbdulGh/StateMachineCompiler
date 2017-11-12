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

string ControlFlowGraph::destroyStructureAndGetFinalSource() //todo finish this
{
    /*
    bool changes = true;
    while (changes)
    {
        changes = false;
        unordered_map<string, unique_ptr<CFGNode>>& nodes = getCurrentNodes();
        auto it = nodes.begin();
        while (it != nodes.end())
        {
            CFGNode* current = it->second.get();
            if (!current->isLastNode() && current->getInstrs().empty() && current->getCompSuccess() == nullptr)
            {
                for (auto& parent : current->getPredecessorVector())
                {
                    if (parent->getCompSuccess() != nullptr && (current->getName() == parent->getCompSuccess()->getName()))
                    {
                        if (current->getCompFail() == nullptr) //returns
                        {
                            if (parent->getCompFail() == nullptr)
                            {
                                parent->setCompSuccess(nullptr);
                                parent->setComp(nullptr);
                            }
                            else
                            {
                                JumpOnComparisonCommand* comp = parent->getComp();
                                comp->op = Relations::negateRelop(comp->op);
                                comp->setData(parent->getCompFail()->getName());
                                parent->setCompSuccess(parent->getCompFail());
                                parent->setCompFail(nullptr);
                            }
                        }
                    }
                    else
                    {
                        if (parent->getCompFail() == nullptr)
                        {
                            parent->getParentFunction()->removeReturnSuccessor(current->getName());
                        }
                        else
                        {
                            if (parent->getCompFail()->getName() != current->getName()) throw "not good";
                            parent->setCompFail(current->getCompFail());
                        }
                    }
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

                if (current->getCompFail() != nullptr) current->getCompFail()->removeParent(current);
                it = nodes.erase(it);
                changes=true;
            }
            else
            {
                auto pit = current->getPredecessorMap().begin();
                while (pit != current->getPredecessorMap().end())
                {
                    if (pit->second->swallowNode(current))
                    {
                        changes = true;
                        pit = current->getPredecessorMap().erase(pit);
                    }
                    else ++pit;
                }
                if ((!current->isLastNode() || current->getPredecessorMap().empty()) && current->getPredecessorMap().empty())
                {
                    if (current->isLastNode())
                    {
                        CFGNode* onlyParent = current->getPredecessorMap().begin()->second;
                        onlyParent->setLast(true);
                        current->getParentFunction()->setLastNode(onlyParent);
                        current->setLast(false);
                    }
                    current->prepareToDie();
                    it = nodes.erase(it);
                    changes = true;
                }
                else ++it;
            }
        }
    }*/

    struct SourceNode
    {
    private:
        vector<AbstractCommand*> toDelete;
    public:
        const string& name;
        vector<SourceNode*> predecessors;
        vector<SourceNode*> successors;
        vector<AbstractCommand*> commands;

        SourceNode(unique_ptr<CFGNode>& toCopy): name(toCopy->getName())
        {
            for (auto& ptr: toCopy->getInstrs()) commands.push_back(ptr.get());
            if (toCopy->getComp() != nullptr) commands.push_back(toCopy->getComp());
            if (toCopy->getCompFail() != nullptr)
            {
                AbstractCommand* jocc = new JumpCommand(toCopy->getCompFail()->getName(), toCopy->getJumpline());
                commands.push_back(jocc);
                toDelete.push_back(jocc);
            }
            else
            {
                AbstractCommand* ret = new ReturnCommand(toCopy->getJumpline());
                commands.push_back(ret);
                toDelete.push_back(ret);
            }
        }

        ~SourceNode()
        {
            for (AbstractCommand* bye: toDelete) delete bye;
        }

        void putSource(stringstream& source)
        {
            source << name << "\n";
            for (auto& instr : commands) source << instr->translation();
            source << "end\n\n";
        }
    };

    map<string, unique_ptr<SourceNode>> outputMap;
    for (auto& pair : currentNodes) outputMap[pair.first] = make_unique<SourceNode>(pair.second);
    for (auto& pair : currentNodes)
    {
        unique_ptr<SourceNode>& sn = outputMap[pair.first];
        for (auto& predpair : pair.second->getPredecessorMap()) sn->predecessors.push_back(outputMap[predpair.first].get());
        for (auto& succ : pair.second->getSuccessorVector()) sn->successors.push_back(outputMap[succ->getName()].get());
    }
    
    for (auto& ptrpair: outputMap)
    {
        unique_ptr<SourceNode>& sn = ptrpair.second;
        for (SourceNode* parent : sn->predecessors)
        {
            auto it = find_if(parent->successors.begin(), parent->successors.end(),
            [&, parent] (SourceNode* polonger) {return polonger->name == sn->name;});
            if (it == parent->successors.end())
            {
                printf("%s claims to be a child of %s\n----------\n%s\n--------\n",
                sn->name.c_str(), parent->name.c_str(), getStructuredSource().c_str());
                auto& debug = parent->successors;
                throw "bad";
            }
        }
        for (SourceNode* succ : sn->successors)
        {
            auto it = find_if(succ->predecessors.begin(), succ->predecessors.end(),
                              [&, succ] (SourceNode* polonger) {return polonger->name == sn->name;});
            if (it == succ->successors.end()) throw "bad";
        }
    }

    stringstream out;
    for (auto& ptrpair: outputMap) ptrpair.second->putSource(out);

    //clear CFG
    first = last = nullptr;
    currentNodes.clear();
    outputMap.clear();

    return out.str();

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