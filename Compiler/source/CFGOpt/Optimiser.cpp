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
                if (last->getName() == current->getName())
                {
                    if (current->getPredecessors().size() != 1) ++pair;
                    else
                    {
                        shared_ptr<CFGNode> pred = current->getPredecessors().cbegin()->second;
                        if (!pred->swallowNode(current)) ++pair;
                        else
                        {
                            controlFlowGraph.setLast(pred->getName());
                            current->getParentFunction()->setLastNode(pred);
                            pair = nodes.erase(pair);
                        }
                    }
                    continue;
                }

                vector<shared_ptr<AbstractCommand>>& instructionList = current->getInstrs();
                unordered_map<string, NodePointer>& preds = current->getPredecessors();

                if (instructionList.empty() && current->getCompSuccess() == nullptr && current->getCompFail() != nullptr)
                {
                    current->replacePushes(current->getCompFail()->getName());
                    for (auto parentit : preds)
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
                    current->prepareToDie();
                    printf("deleting %s\n", current->getName().c_str());
                    if (current->getName() == "F_loopbody_0") printf("%s\n--\n", controlFlowGraph.getSource().c_str());
                    pair = nodes.erase(pair);
                }
                else ++pair;
            }
        }
    }
}