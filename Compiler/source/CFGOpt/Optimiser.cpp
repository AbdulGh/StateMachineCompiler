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

                if (current->isLastNode())
                {
                    if (current->getPredecessors().size() != 1) ++pair;
                    else
                    {
                        shared_ptr<CFGNode> pred = current->getPredecessors().cbegin()->second;
                        if (!pred->swallowNode(current)) ++pair;
                        else
                        {
                            current->getParentFunction()->setLastNode(pred);
                            current->prepareToDie();
                            pair = nodes.erase(pair);
                        }
                    }
                    continue;
                }

                vector<shared_ptr<AbstractCommand>>& instructionList = current->getInstrs();
                unordered_map<string, NodePointer>& preds = current->getPredecessors();

                if (instructionList.empty() && current->getCompSuccess() == nullptr)
                {
                    //if its just an unconditional jump
                    if (current->getCompFail() != nullptr)
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
                            else if (parent->isLastNode())
                            {
                                FunctionSymbol* ppf = parent->getParentFunction();
                                ppf->removeReturnSuccessor(current->getName());
                                ppf->addReturnSuccessor(current->getCompFail().get());
                            }
                            else throw "couldn't find self in parent";
                        }
                        preds.clear();
                    }
                }

                else if (preds.size() == 1)
                {
                    shared_ptr<CFGNode> parent = preds.cbegin()->second;
                    if (parent->swallowNode(current))
                    {
                        if (parent->isLastNode()) current->getParentFunction()->giveNodesTo(parent->getParentFunction());
                        preds.clear();
                    }
                }

                else if (instructionList.size() <= 4)
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
                    pair = nodes.erase(pair);
                }
                else ++pair;
            }
        }
    }
}