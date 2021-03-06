#include <iostream>

#include "State.h"
#include "FSM.h"
#include "Command.h"

using namespace std;

class FSM;
State::State(string str, FSM& owner):
    parent(owner),
    name(move(str)) {}

void State::run()
{
    mnextState = -1;
    for (unique_ptr<AbstractCommand>& command : instructions)
    {
        command->execute();
        if (command->changesState())
        {
            mnextState = command->getNextState();
            return;
        }
    }
}

int State::nextState() const
{
    return mnextState;
}


const std::string &State::getName() const
{
    return name;
}

const vector<unique_ptr<AbstractCommand>>& State::getInstructions() const
{
    return instructions;
}

void State::setInstructions(vector<unique_ptr<AbstractCommand>> in)
{
    instructions = move(in);
}
