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
            vector<shared_ptr<AbstractCommand>>& instructionList = current->getInstrs();
            bool incremented = false;
            if (instructionList.size() == 0 &&
                current->getComp() == nullptr && current->getCompFail() != nullptr) //node consists of a single unconditional jump (not return)
            {
                NodePointer replaceWith = current->getCompFail();
                replaceWith->removeParent(current);

                for (auto p : current->getPredecessors())
                {
                    NodePointer replacing = p.second;
                    NodePointer succ = replacing->getCompSuccess();
                    if (succ != nullptr && succ->getName() == current->getName())
                    {
                        replacing->getComp()->setData(replaceWith->getName());
                        replacing->setCompSuccess(replaceWith);
                        replaceWith->addParent(replacing);
                    }

                    NodePointer fail = replacing->getCompFail();
                    if (fail != nullptr && fail->getName() == current->getName())
                    {
                        replacing->setCompFail(replaceWith);
                        replaceWith->addParent(replacing);
                    }
                }
                pair = nodes.erase(pair);
                incremented = true;
            }
            if (!incremented) ++pair;
        }
    }
}