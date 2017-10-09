#include <algorithm>

#include "Optimiser.h"
#include "../Command.h"

using namespace std;

typedef shared_ptr<CFGNode> NodePointer;

namespace Optimise
{
    void optimise(SymbolTable& symbolTable, FunctionTable& functionTable, ControlFlowGraph& controlFlowGraph)
    {
        bool changes = true;
        while (changes)
        {
            changes = false;
            Optimise::collapseSmallStates(controlFlowGraph, functionTable);
            for (const auto& node : controlFlowGraph.getCurrentNodes())
            {
                if (node.second->constProp()) changes = true;
            }
        }
    }

    void collapseSmallStates(ControlFlowGraph& controlFlowGraph, FunctionTable& functionTable)
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
                            changes = true;
                        }
                    }
                    continue;
                }

                vector<unique_ptr<AbstractCommand>>& instructionList = current->getInstrs();
                unordered_map<string, NodePointer>& preds = current->getPredecessors();

                //if its just an unconditional jump
                if (instructionList.empty() && current->getCompSuccess() == nullptr && current->getCompFail() != nullptr)
                {
                    current->replacePushes(current->getCompFail()->getName());
                }
                else if (preds.size() == 1)
                {
                    shared_ptr<CFGNode> parent = preds.cbegin()->second;
                    if (parent->swallowNode(current))
                    {
                        string oldFSPrefix = current->getParentFunction()->getPrefix();
                        if (current->isFirstNode()) current->getParentFunction()->giveNodesTo(parent->getParentFunction());
                        functionTable.removeFunction(oldFSPrefix);
                        preds.clear();
                        changes = true;
                    }
                }
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