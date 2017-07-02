#include "Optimiser.h"
#include "../Command.h"

using namespace std;

void Optimiser::optimise()
{
    collapseSmallStates();
    propogateConstants();
}

void Optimiser::collapseSmallStates()
{
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

void Optimiser::replaceJumps(std::string sName, std::string replaceWith)
{
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
                if (ACPtr->getEffect() == CommandSideEffect::JUMP && ACPtr->getData() == sName) ACPtr->setData(replaceWith);
                else it++;
            }
        }
    }
}

void Optimiser::propogateConstants()
{
    //todo
}