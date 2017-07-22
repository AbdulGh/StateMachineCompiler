#include "Optimiser.h"
#include "../Command.h"

using namespace std;

typedef shared_ptr<CFGNode> NodePointer;

namespace Optimise
{
    void optimise(SymbolTable& symbolTable, ControlFlowGraph& controlFlowGraph)
    {
        Optimise::collapseSmallStates(controlFlowGraph);
        Optimise::propogateConstants(symbolTable, controlFlowGraph);
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
                current->getComp() == nullptr) //node consists of a single unconditional jump
            {
                NodePointer replaceWith = current->getCompFail();
                replaceWith->removeParent(current);
                string delet = pair->first;

                //vector<NodePointer>::iterator replacing = current->getPredecessors().begin();
                //while (replacing != current->getPredecessors().end())
                for (NodePointer replacing : current->getPredecessors())
                {
                    NodePointer succ = replacing->getCompSuccess();
                    if (succ != nullptr && succ->getName() == current->getName())
                    {
                        replacing->getComp()->setData(replaceWith->getName());
                        replacing->setCompSuccess(replaceWith);
                        replaceWith->addParent(replacing);
                    }

                    NodePointer fail = replacing->getCompFail();
                    if (fail == nullptr) //should be fine, just checking
                    {
                        int debug;
                        debug = 5;
                    }
                    else if (fail->getName() == current->getName())
                    {
                        replacing->setCompFail(replaceWith);
                        replaceWith->addParent(replacing);
                    }
                }
                delet = pair->first;
                pair = nodes.erase(pair);
                incremented = true;
            }
            /*else if (current->getCompSuccess() == nullptr && current->getPredecessors().size() == 1)
            {
                //node consists of a single instruction followed by an unconditional jump
                NodePointer pred = current->getPredecessors()[0];
                pred->getInstrs().push_back(current->getInstrs()[0]);
            }*/
            if (!incremented) ++pair;
        }
    }

    void propogateConstants(SymbolTable& symbolTable, ControlFlowGraph& controlFlowGraph)
    {
        //todo
    }
}