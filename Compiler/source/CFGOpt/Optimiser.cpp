#include <algorithm>

#include "Optimiser.h"
#include "../Command.h"

using namespace std;

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
        unordered_map<string, unique_ptr<CFGNode>>& nodes = controlFlowGraph.getCurrentNodes();
        bool changes = true;
        while (changes)
        {
            changes = false;
            auto pair = nodes.begin();
            while (pair != nodes.end())
            {
                CFGNode* current = pair->second.get();

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
                        CFGNode* pred = current->getPredecessors().cbegin()->second;
                        if (!pred->swallowNode(current)) ++pair;
                        else
                        {
                            current->getParentFunction()->giveNodesTo(pred->getParentFunction()); //does nothing if they're the same
                            pred->getParentFunction()->setLastNode(pred);
                            current->prepareToDie();
                            pair = nodes.erase(pair);
                            changes = true;
                        }
                    }
                    continue;
                }

                vector<unique_ptr<AbstractCommand>>& instructionList = current->getInstrs();
                unordered_map<string, CFGNode*>& preds = current->getPredecessors();

                //if its just an unconditional jump
                if (instructionList.empty() && current->getCompSuccess() == nullptr && current->getCompFail() != nullptr)
                {
                    current->replacePushes(current->getCompFail()->getName());
                }
                else if (preds.size() == 1)
                {
                    CFGNode* parent = preds.cbegin()->second;
                    if (parent->swallowNode(current))
                    {
                        if (current->isFirstNode())
                        {
                            current->getParentFunction()->giveNodesTo(parent->getParentFunction());
                            functionTable.removeFunction(current->getParentFunction()->getPrefix());
                        }
                        preds.clear();
                        changes = true;
                    }
                }
                auto parentit = preds.begin();
                while (parentit != preds.end())
                {
                    CFGNode* swallowing = parentit->second;

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