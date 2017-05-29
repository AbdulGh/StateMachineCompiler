#include "State.h"
#include <iostream>

using namespace std;

State::State(string str)
{
    name = str;
}

void State::run()
{
    for (shared_ptr<AbstractCommand> &command : instructions)
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

const vector<shared_ptr<AbstractCommand>> &State::getInstructions() const
{
    return instructions;
}

void State::setInstructions(const vector<shared_ptr<AbstractCommand>> &in)
{
    instructions = move(in);
}
