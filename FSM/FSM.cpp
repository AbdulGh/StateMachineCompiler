#include <iostream>
#include <memory>

#include "FSM.h"
#include "State.h"

using namespace std;

void FSM::setStates(vector<unique_ptr<State>> st)
{
    int debug = st.size();
    this->states = move(st);
}

void FSM::run()
{
    if (states.empty()) throw "need at least one state";
    int currentStateNum = 0;
    while (currentStateNum != -1)
    {
        auto& currentState = states.at(currentStateNum);
        currentState->run();
        currentStateNum = currentState->nextState();
    }
}