#include "Optimiser.h"
#include "../Command.h"

using namespace std;

typedef shared_ptr<CFGNode> NodePointer;

namespace Optimise
{
    void optimise(SymbolTable& symbolTable, ControlFlowGraph& controlFlowGraph)
    {
        Optimise::collapseSmallStates(controlFlowGraph);
    }

    void collapseSmallStates(ControlFlowGraph& controlFlowGraph)
    {
        unordered_map<string, NodePointer> &nodes = controlFlowGraph.getCurrentNodes();
        unordered_map<string, NodePointer>::iterator pair = nodes.begin();
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
                last = last->getPredecessors().cbegin()->second;
            }

            vector<shared_ptr<AbstractCommand>> &instructionList = current->getInstrs();
            if (instructionList.size() <= 4) //is small
            {
                unordered_map<string, NodePointer>& preds = current->getPredecessors();
                unordered_map<string, NodePointer>::iterator parentit = preds.begin();
                while (parentit != preds.end())
                {
                    NodePointer swallowing = parentit->second;
                    if (!swallowing->swallowNode(current)) ++parentit;
                    else parentit = preds.erase(parentit);
                }
                if (preds.size() == 0)
                {
                    if (current->getCompFail() != nullptr) current->getCompFail()->removeParent(current);
                    if (current->getCompSuccess() != nullptr) current->getCompSuccess()->removeParent(current);
                    pair = nodes.erase(pair);
                }
                else ++pair;
            }
            else ++pair;
        }
    }
}