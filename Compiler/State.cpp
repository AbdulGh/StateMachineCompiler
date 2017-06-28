#include <iostream>

#include "State.h"
#include "Command.h"

using namespace std;

State::State(string str):
    name(str) {}

/*State::~State()
{
    for (shared_ptr<AbstractCommand> a : instructions)  a.reset();
}*/

const std::string& State::getName() const
{
    return name;
}

vector<shared_ptr<AbstractCommand>>& State::getInstructions()
{
    return instructions;
}

void State::setInstructions(const vector<std::shared_ptr<AbstractCommand>> &in)
{
    instructions = move(in);
}

