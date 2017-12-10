#include <algorithm>

#include "Optimiser.h"
#include "DataFlow.h"
#include "LengTarj.h"

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
        DataFlow::AssignmentPropogationDataFlow(controlFlowGraph, symbolTable).worklist();
        DataFlow::LiveVariableDataFlow(controlFlowGraph, symbolTable).worklist();
    }

    void collapseSmallStates(ControlFlowGraph& controlFlowGraph, FunctionTable& functionTable)
    {
        unordered_map<string, unique_ptr<CFGNode>>& nodes = controlFlowGraph.getCurrentNodes();
        bool changes = true;
        while (changes)
        {
            changes = false;
            auto pair = nodes.begin();
            while (pair != nodes.end()) //just get rid of unconditional jumps first
            {
                CFGNode* current = pair->second.get();
                if (current->isLastNode())
                {
                    ++pair;
                    continue;
                }
                vector<unique_ptr<AbstractCommand>>& instructionList = current->getInstrs();
                if (instructionList.empty() && current->getCompSuccess() == nullptr)
                {
                    if (current->getCompFail() != nullptr) current->replacePushes(current->getCompFail()->getName());
                    else current->removeCallsTo();

                    for (const auto& parentit : current->getPredecessorMap())
                    {
                        if (!parentit.second->swallowNode(current))
                        {
                            printf("%s", controlFlowGraph.getStructuredSource().c_str());
                            throw "should swallow";
                        }
                    }
                    current->prepareToDie();
                    pair = nodes.erase(pair);
                    changes = true;
                }
                else ++pair;
            }

            pair = nodes.begin();
            while (pair != nodes.end())
            {
                CFGNode* current = pair->second.get();
                vector<unique_ptr<AbstractCommand>>& instructionList = current->getInstrs();
                const unordered_map<string, CFGNode*>& preds = current->getPredecessorMap();

                if (current->getName() == controlFlowGraph.getFirst()->getName())
                {
                    ++pair;
                    continue;
                }

                if (current->isLastNode())
                {
                    if (preds.size() != 1) ++pair;
                    else
                    {
                        CFGNode* pred = preds.cbegin()->second;
                        if (pred->getName() == current->getName()) ++pair;
                        else
                        {
                            if (pred->swallowNode(current))
                            {
                                current->setLast(false);
                                if (current->getName() == controlFlowGraph.getLast()->getName())
                                {
                                    controlFlowGraph.setLast(pred->getName());
                                }
                                pred->getParentFunction()->setLastNode(pred);
                                pred->setLast();
                                current->prepareToDie();
                                pair = nodes.erase(pair);
                                changes = true;
                            }
                            else ++pair;
                        }
                    }
                    continue;
                }
                if (preds.size() == 1)
                {
                    CFGNode* parent = preds.cbegin()->second;
                    if (parent->swallowNode(current))
                    {
                        current->clearPredecessors();
                        changes = true;
                    }
                }
                if (current->noPreds())
                {
                    current->prepareToDie();
                    pair = nodes.erase(pair);
                    changes = true;
                }
                else ++pair;
            }
        }
    }
}