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
            if (pair->first == "pop")
            {
                ++pair;
                continue;
            }

            NodePointer current = pair->second;

            if (current->getInstrs().size() == 0 &&
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
                    string delet = pair->first;
                }
                delet = pair->first;
                pair = nodes.erase(pair);
            }
            else ++pair;
        }
    }

    void propogateConstants(SymbolTable& symbolTable, ControlFlowGraph& controlFlowGraph)
    {
        //todo
    }

        /*
        bool changes = true;

        while (changes)
        {
            changes = false;
            for (auto &f : functionTable)
            {
                vector<State>& states = f.second->getFinStates();
                vector<State>::iterator it = states.begin();
                while (it != states.end())
                {
                    State s = *it;
                    vector<shared_ptr<AbstractCommand>>& instrs = s.getInstructions();

                    if (instrs.size() == 1 && instrs.at(0)->getEffect() == CommandSideEffect::JUMP)
                    {
                        replaceJumps(s.getName(), instrs.at(0)->getData());
                        states.erase(it);
                        changes = true;
                    }
                    else it++;
                }
            }
        }
    }

    void Optimise::replaceJumps(std::string sName, std::string replaceWith)
    {
        /*
        for (auto &f : functionTable)
        {
            vector<State>& states = f.second->getFinStates();
            for (State s : states)
            {
                vector<shared_ptr<AbstractCommand>>& instructions = s.getInstructions();
                vector<shared_ptr<AbstractCommand>>::iterator it = instructions.begin();
                while (it != instructions.end())
                {
                    shared_ptr<AbstractCommand> ACPtr = (*it);
                    if ((ACPtr->getEffect() == CommandSideEffect::JUMP || ACPtr->getEffectFlag() == CommandSideEffect::JUMP)
                        && ACPtr->getData() == sName) ACPtr->setData(replaceWith);
                    else it++;
                }
            }
        }

    }*/


}