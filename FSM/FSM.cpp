#include <iostream>
#include <memory>

#include "FSM.h"
#include "State.h"

using namespace std;

void FSM::setStates(vector<unique_ptr<State>> st)
{
    this->states = move(st);
}

void FSM::run()
{
    int currentStateNum = 0;
    while (currentStateNum != -1)
    {
        auto& currentState = states.at(currentStateNum);
        currentState->run();
        currentStateNum = currentState->nextState();
    }
}