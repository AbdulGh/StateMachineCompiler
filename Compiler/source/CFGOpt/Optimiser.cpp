#include <algorithm>

#include "Optimiser.h"
#include "../Command.h"

using namespace std;

typedef shared_ptr<CFGNode> NodePointer;

namespace Optimise
{
    void optimise(SymbolTable& symbolTable, ControlFlowGraph& controlFlowGraph)
    {
        bool changes = true;
        while (changes)
        {
            changes = false;
            Optimise::collapseSmallStates(controlFlowGraph);
            for (auto node : controlFlowGraph.getCurrentNodes())
            {
                if (node.second->constProp()) changes = true;
            }
        }
    }

    void collapseSmallStates(ControlFlowGraph& controlFlowGraph)
    {
        unordered_map<string, NodePointer>& nodes = controlFlowGraph.getCurrentNodes();
        bool changes = true;
        while (changes)
        {
            changes = false;
            auto pair = nodes.begin();
            while (pair != nodes.end())
            {
                NodePointer current = pair->second;
                if (current->getName() == controlFlowGraph.getFirst()->getName())
                {
                    ++pair;
                    continue;
                }
                NodePointer last = controlFlowGraph.getLast();
                bool currentIsLast = last->getName() == current->getName();
                if (currentIsLast && last->getPredecessors().size() != 1)
                {
                    ++pair;
                    continue;
                }

                vector<shared_ptr<AbstractCommand>>& instructionList = current->getInstrs();
                unordered_map<string, NodePointer>& preds = current->getPredecessors();

                if (instructionList.empty() && current->getCompSuccess() == nullptr && current->getCompFail() != nullptr)
                {
                    current->replacePushes(current->getCompFail()->getName());
                    current->getCompFail()->removeParent(current->getName());
                    for (const auto& parentit : preds)
                    {
                        shared_ptr<CFGNode> parent = parentit.second;
                        if (parent->getCompSuccess() != nullptr && parent->getCompSuccess()->getName() == current->getName())
                        {
                            parent->setCompSuccess(current->getCompFail());
                            continue;
                        }
                        else if (parent->getCompFail() != nullptr && parent->getCompFail()->getName() == current->getName())
                        {
                            parent->setCompFail(current->getCompFail());
                            continue;
                        }
                        else
                        {
                            bool found = false;
                            vector<shared_ptr<CFGNode>>& parentRetSuccessors = parent->getReturnSuccessors();
                            for (auto iterator = parentRetSuccessors.begin();
                                 iterator != parentRetSuccessors.end(); iterator++)
                            {
                                shared_ptr<CFGNode> retPointer = *iterator;
                                if (retPointer->getName() == current->getName())
                                {
                                    found = true;
                                    parentRetSuccessors.erase(iterator);
                                    break;
                                }
                            }
                            if (!found) throw "couldn't find self in parent";
                            parentRetSuccessors.push_back(current->getCompFail());
                        }
                    }
                    preds.clear();
                }

                else if (instructionList.size() <= 4 || preds.size() == 1) //is small
                {
                    auto parentit = preds.begin();
                    while (parentit != preds.end())
                    {
                        NodePointer swallowing = parentit->second;

                        if (!swallowing->swallowNode(current)) ++parentit;
                        else
                        {
                            changes = true;
                            parentit = preds.erase(parentit);
                        }
                    }
                }
                if (preds.empty())
                {
                    if (current->getCompFail() != nullptr) current->getCompFail()->removeParent(current);
                    if (current->getCompSuccess() != nullptr)
                    {
                        current->getCompSuccess()->removeParent(current);
                    }
                    if (currentIsLast)
                    {
                        controlFlowGraph.setLast(last->getPredecessors().cbegin()->second->getName()); //->.*(().->)->*()->;
                    }
                    current->removePushes();
                    current->clearReturnSuccessors();
                    pair = nodes.erase(pair);
                }
                else ++pair;
            }
        }
    }
}