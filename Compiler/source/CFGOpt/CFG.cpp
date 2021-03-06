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

NodeMapIterator ControlFlowGraph::removeNode(NodeMapIterator it)
{
    if (it == currentNodes.end()) throw std::runtime_error("Check");
    unique_ptr<CFGNode>& nodePointer = it->second;
    if (nodePointer->isLastNode())
    {
        if (nodePointer->getPredecessorMap().size() != 1) throw runtime_error("Can't replace last node");
        CFGNode* newLast = last->getPredecessorMap().cbegin()->second;
        nodePointer->getParentFunction()->mergeInto(newLast->getParentFunction());
        nodePointer->getParentFunction()->setLastNode(newLast);
        if (last->getName() == nodePointer->getName()) last = newLast;
    }
    return currentNodes.erase(it);
}

NodeMapIterator ControlFlowGraph::removeNode(string name)
{
    auto it = currentNodes.find(name);
    return removeNode(it);
}

const FunctionSymbol& ControlFlowGraph::getMain() const
{
    const FunctionSymbol* fs = functionTable.getFunction("main");
    if (!fs) throw runtime_error("main should be defined");
    return *fs;
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

string ControlFlowGraph::destroyStructureAndGetFinalSource()
{
    bool changes = true;

    class SourceNode;
    static map<string, unique_ptr<SourceNode>> outputMap;
    outputMap.clear();

    class SourceNode //just a less structured CFGNode
    {
    private:
        vector<unique_ptr<AbstractCommand>> instructions;
        vector<SourceNode*> predecessors;
        vector<SourceNode*> successors;
        bool first;
    public:
        const string name;

        SourceNode(unique_ptr<CFGNode>& toCopy): name(toCopy->getName())
        {
            for (auto& ptr: toCopy->getInstrs()) instructions.push_back(ptr->clone());
            if (toCopy->getComp() != nullptr) instructions.push_back(toCopy->getComp()->clone());
            if (toCopy->getCompFail() != nullptr)
            {
                instructions.push_back
                        (make_unique<JumpCommand>(toCopy->getCompFail()->getName(), toCopy->getJumpline()));
            }
            else instructions.push_back(make_unique<ReturnCommand>(toCopy->getJumpline()));
            first = toCopy->isFirstNode();
        }

        bool isFirst() const {return first;}

        vector<unique_ptr<AbstractCommand>>& getInstructions()
        {
            return instructions;
        }

        vector<SourceNode*>& getPredecessors()
        {
            return predecessors;
        }

        vector<SourceNode*>& getSuccessors()
        {
            return successors;
        }

        void addPredecessor(SourceNode* pred)
        {
            predecessors.push_back(pred);
        }

        void addSuccessor(SourceNode* succ)
        {
            successors.push_back(succ);
        }

        void loseParent(SourceNode* parent)
        {
            auto it = find_if(predecessors.begin(), predecessors.end(),
                [&, parent] (const SourceNode* con) {return parent->name == con->name;});
            if (it == predecessors.end()) throw std::runtime_error("could not find parent");
            predecessors.erase(it);
        }

        void loseKid(SourceNode* kid)
        {
            auto it = find_if(successors.begin(), successors.end(),
                              [&, kid] (const SourceNode* con) {return kid->name == con->name;});
            if (it == successors.end()) throw std::runtime_error("could not find successor");
            successors.erase(it);
        }

        vector<SourceNode*>::iterator eraseParent(vector<SourceNode*>::iterator it)
        {
            return predecessors.erase(it);
        }

        inline void loseKids()
        {
            for (auto& succ: successors) succ->loseParent(this);
            successors.clear();
        }

        bool tryBypass()
        {
            if (instructions.empty()) throw std::runtime_error("shouldnt happen");

            unique_ptr<AbstractCommand>& lastInstr = instructions.back();
            if (lastInstr->getType() == CommandType::JUMP
                && lastInstr->getString() != "return"
                && lastInstr->getString() != name)
            {
                unique_ptr<SourceNode>& swallowing = outputMap[lastInstr->getString()];
                if (swallowing == nullptr) throw std::runtime_error("cant find jumped to node");
                else if (swallowing->predecessors.size() > 1) return false; //todo improve this by detecting cycles

                vector<unique_ptr<AbstractCommand>>& swallowingInstrs = swallowing->instructions;
                if (swallowingInstrs.empty()) throw std::runtime_error("this shouldn't be");
                else if(swallowingInstrs.back()->getType() == CommandType::JUMP
                        && swallowingInstrs.back()->getString() == name) return false;

                instructions.pop_back();

                const string& swallowingName = swallowing->name;
                auto swallowingIt = find_if(successors.begin(), successors.end(),
                [&, swallowingName] (const auto& child) {return child->name == swallowingName;});
                if (swallowingIt == successors.end()) throw std::runtime_error("child should be in");
                (*swallowingIt)->loseParent(this);
                successors.erase(swallowingIt);

                for (const auto& newInstruction: swallowingInstrs) instructions.push_back(newInstruction->clone());
                for (const auto& newSucc: swallowing->successors)
                {
                    successors.push_back(newSucc);
                    newSucc->addPredecessor(this);
                }

                return true;
            }
            return false;
        }

        void putSource(stringstream& source)
        {
            source << name << "\n";
            for (auto& instr : instructions) source << instr->translation();
            source << "end\n\n";
        }
    };
    for (auto& pair : currentNodes) outputMap[pair.first] = make_unique<SourceNode>(pair.second);
    for (auto& pair : currentNodes)
    {
        unique_ptr<SourceNode>& sn = outputMap[pair.first];
        for (auto& predpair : pair.second->getPredecessorMap())
        {
            unique_ptr<SourceNode>& pred = outputMap[predpair.first];
            sn->addPredecessor(pred.get());
            pred->addSuccessor(sn.get());
        }
    }

    changes = true;
    while (changes)
    {
        changes = false;
        auto it = outputMap.begin();
        while (it != outputMap.end())
        {
            unique_ptr<SourceNode>& sn = it->second;

            if (sn->getInstructions().size() == 1)
            {
                const unique_ptr<AbstractCommand>& onlyInstr = *(sn->getInstructions().begin());
                if (onlyInstr->getType() == CommandType::JUMP)
                {
                    auto& snPreds = sn->getPredecessors();
                    vector<SourceNode*>::iterator parentIt = snPreds.begin();
                    while (parentIt != snPreds.end())
                    {
                        SourceNode* parentNode = *parentIt;
                        vector<unique_ptr<AbstractCommand>>& parentInstructions = parentNode->getInstructions();
                        bool found = false;
                        for (unsigned int parentInstIndex = 0; parentInstIndex < parentInstructions.size(); ++parentInstIndex)
                        {
                            unique_ptr<AbstractCommand>& instr = parentInstructions.at(parentInstIndex);
                            if (instr->getString() == sn->name)
                            {
                                if (onlyInstr->getString() == "return")
                                {
                                    parentInstructions[parentInstIndex] = make_unique<ReturnCommand>(instr->getLineNum());
                                }
                                else instr->setString(onlyInstr->getString());
                                found = true;
                            }
                        }
                        if (!found && (onlyInstr->getString() != "return" || parentInstructions.empty()
                                       || (*parentInstructions.rend())->getString() != "return"))
                        {
                            throw std::runtime_error("bad parent");
                        }

                        parentNode->loseKid(sn.get());
                        for (SourceNode* succ: sn->getSuccessors())
                        {
                            parentNode->addSuccessor(succ);
                            succ->addPredecessor(parentNode);
                        }
                        parentIt = sn->eraseParent(parentIt);
                        changes = true;
                    }
                }
            }
            if (sn->getPredecessors().empty() && !sn->isFirst())
            {
                sn->loseKids();
                it = outputMap.erase(it);
            }
            else
            {
                if (sn->tryBypass()) changes = true;
                ++it;
            }
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
        if (n->getParentFunction()->getIdent() != currentFunc) //functions cant have empty identifier
        {
            if (!currentFunc.empty())  outs << "}\n";
            currentFunc = n->getParentFunction()->getIdent();
            outs << "subgraph cluster_" << currentFunc << "{\n";
            outs << "label=\"" << currentFunc << "\";\n";
        }
        outs << n->getDotNode();
        outs << '\n';
    }
    outs << "}\n";

    //if you define edges within subgraphs, nodes get placed in the wrong clusters
    for (auto& it : ordered)
    {
        outs << it.second->getDotEdges();
    }
    outs << "}\n";
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
    if (it == currentNodes.end()) throw std::runtime_error("Check");
    first = it->second.get();
}

void ControlFlowGraph::setLast(const string& lastName)
{
    auto it = currentNodes.find(lastName);
    if (it == currentNodes.end()) throw std::runtime_error("Check");
    last = it->second.get();
}