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
                vector<unique_ptr<AbstractCommand>>& instructionList = current->getInstrs();
                unordered_map<string, CFGNode*>& preds = current->getPredecessors();

                if (current->getName() == controlFlowGraph.getFirst()->getName())
                {
                    ++pair;
                    continue;
                }
                
                /*if (current->isFirstNode())
                {
                    if (current->isLastNode()) //only one node - get rid of function
                    {
                        if (current->getCompSuccess() != nullptr && current->getCompFail() != nullptr) throw "check";
                        if (instructionList.empty())
                        {
                            current->removePushes();

                            for (const auto& parentit : preds)
                            {
                                if (!parentit.second->swallowNode(current)) throw "should swallow";
                            }
                            preds.clear();
                        }
                        else
                        {
                            auto parentit = preds.begin();
                            while (parentit != preds.end())
                            {
                                if (parentit->second->swallowNode(current))
                                {
                                    parentit = preds.erase(parentit);
                                    changes = true;
                                }
                                else ++parentit;
                            }
                        }
                    }

                    if (preds.empty()) functionTable.removeFunction(current->getParentFunction()->getIdent());
                    else ++pair;
                }*/

                if (current->isLastNode())
                {
                    if (preds.size() != 1) ++pair;
                    else
                    {
                        CFGNode* pred = preds.cbegin()->second;
                        if (!pred->swallowNode(current)) ++pair;
                        else
                        {
                            current->removePushes();
                            //current->getParentFunction()->giveNodesTo(pred->getParentFunction()); //does nothing if they're the same
                            pred->getParentFunction()->setLastNode(pred);
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

                    for (const auto& parentit : current->getPredecessors())
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
                        /*if (current->isFirstNode())
                        {
                            current->getParentFunction()->giveNodesTo(parent->getParentFunction());
                            functionTable.removeFunction(current->getParentFunction()->getPrefix());
                        }*/
                        preds.clear();
                        changes = true;
                    }
                }
                else if (preds.size() > 1)
                {
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

    void destroyBinaryCFGStructure(ControlFlowGraph& controlFlowGraph) //todo
    {
        unordered_map<string, unique_ptr<CFGNode>>& nodes = controlFlowGraph.getCurrentNodes();
        bool changes = true;
        while (changes)
        {
            changes = false;
            auto pair = nodes.begin();
            while (pair != nodes.end())
            {
                CFGNode *current = pair->second.get();
                vector<unique_ptr<AbstractCommand>>& instructionList = current->getInstrs();
                unordered_map<string, CFGNode *>& preds = current->getPredecessors();

                if (instructionList.empty())
                {

                }

            }
        }
    }
}