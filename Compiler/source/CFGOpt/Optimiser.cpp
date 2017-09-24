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
        unordered_map<string, NodePointer> &nodes = controlFlowGraph.getCurrentNodes();
        auto pair = nodes.begin();
        while (pair != nodes.end())
        {
            NodePointer current = pair->second;
            NodePointer last = controlFlowGraph.getLast();
            if (last != nullptr && last->getName() == current->getName())
            {
                if (last->getPredecessors().size() != 1)
                {
                    ++pair;
                    continue;
                }
                controlFlowGraph.setLast(last->getPredecessors().cbegin()->second->getName()); //->.*(().->)->*()->;
            }

            vector<shared_ptr<AbstractCommand>> &instructionList = current->getInstrs();
            if (instructionList.size() <= 4 || current->getPredecessors().size() == 1) //is small
            {
                unordered_map<string, NodePointer>& preds = current->getPredecessors();
                auto parentit = preds.begin();
                while (parentit != preds.end())
                {
                    NodePointer swallowing = parentit->second;
                    if (!swallowing->swallowNode(current)) ++parentit;
                    else parentit = preds.erase(parentit);
                }
                if (preds.empty())
                {
                    if (current->getCompFail() != nullptr) current->getCompFail()->removeParent(current);
                    if (current->getCompSuccess() != nullptr) current->getCompSuccess()->removeParent(current);
                    for (const shared_ptr<CFGNode>& returnSucc : current->getReturnSuccessors())
                    {
                        returnSucc->removeParent(current->getName());
                    }
                    pair = nodes.erase(pair);
                }
                else ++pair;
            }
            else ++pair;
        }
    }
}