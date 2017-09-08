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
            vector<shared_ptr<AbstractCommand>> &instructionList = current->getInstrs();
            bool incremented = false;
            if (instructionList.size() <= 4) //is small
            {
                bool canDelete = true;
                auto parentPair = current->getPredecessors().begin();
                while (parentPair != current->getPredecessors().end())
                {
                    shared_ptr<CFGNode> swallowing = parentPair->second;
                    string name = swallowing->getName();
                    string name2 = parentPair->first;
                    if (swallowing->getName() == "")
                    {
                        int debug;
                        debug = 5; //todo find out why name is going to ""
                    }

                    if (swallowing->swallowNode(current))
                    {
                        canDelete = false;
                        ++parentPair;
                    }
                }
                if (canDelete)
                {
                    nodes.erase(pair);
                    incremented = true;
                }
            }
            if (!incremented) ++pair;
        }
    }
}