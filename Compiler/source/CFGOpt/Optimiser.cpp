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
            while (pair != nodes.end())
            {
                CFGNode* current = pair->second.get();
                vector<unique_ptr<AbstractCommand>>& instructionList = current->getInstrs();
                unordered_map<string, CFGNode*>& preds = current->getPredecessorMap();

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
                        bool currentIsPred = pred->getName() == current->getName();
                        if (!currentIsPred && !pred->swallowNode(current)) ++pair;
                        else
                        {
                            current->removePushes();
                            if (currentIsPred) current->setLast(false);
                            else
                            {
                                current->getParentFunction()->giveNodesTo(pred->getParentFunction()); //does nothing if they're the same
                                pred->getParentFunction()->setLastNode(pred); //ditto
                            }
                            current->prepareToDie();
                            pair = nodes.erase(pair);
                            changes = true;
                        }
                    }
                    continue;
                }

                //if its just an unconditional jump
                if (instructionList.empty() && current->getCompSuccess() == nullptr)
                {
                    if (current->getCompFail() != nullptr) current->replacePushes(current->getCompFail()->getName());
                    else current->removePushes();

                    for (const auto& parentit : current->getPredecessorMap())
                    {
                        if (!parentit.second->swallowNode(current)) throw "should swallow";
                    }
                    preds.clear();
                    changes = true;
                }
                if (preds.size() == 1)
                {
                    CFGNode* parent = preds.cbegin()->second;
                    if (parent->swallowNode(current))
                    {
                        preds.clear();
                        changes = true;
                    }
                }
//                else if (preds.size() > 1)
//                {
//                    //todo make sure no states push themselves onto the stack and return
//                    bool currentSelfSucc = current->getCompSuccess() != nullptr
//                                           && current->getCompSuccess()->getName() == current->getName();
//                    bool currentSelfFail = current->getCompFail() != nullptr
//                                           && current->getCompFail()->getName() == current->getName();
//
//                    if (!currentSelfFail && !currentSelfSucc)
//                    {
//                        auto parentit = preds.begin();
//                        while (parentit != preds.end())
//                        {
//                            CFGNode* swallowing = parentit->second;
//
//                            if (swallowing->getName() == current->getName()
//                                || !swallowing->swallowNode(current)) ++parentit;
//                            else
//                            {
//                                changes = true;
//                                parentit = preds.erase(parentit);
//                            }
//                        }
//                    }
//                }
                if (current->noPreds())
                {
                    current->prepareToDie();
                    pair = nodes.erase(pair);
                }
                else ++pair;
            }
        }
    }
}