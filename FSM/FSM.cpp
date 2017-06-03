#include <iostream>
#include <memory>

#include "FSM.h"
#include "State.h"

using namespace std;

void FSM::setStates(vector<shared_ptr<State>> st)
{
    this->states = st;
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