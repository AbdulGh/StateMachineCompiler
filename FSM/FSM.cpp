#include <iostream>
#include <fstream>
#include <unordered_map>
#include <memory>
#include "FSM.h"

using namespace std;

FSM::FSM(vector<shared_ptr<State>> st)
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