#include <iostream>
#include <memory>

#include "FSM.h"
#include "State.h"

using namespace std;

FSM::FSM(string& filename)
{
    FSMParser(filename, *this).readFSM();
}

void FSM::run()
{
    if (states.empty()) throw "need at least one state";
    int currentStateNum = 0;
    while (currentStateNum != -1)
    {
        auto& currentState = states.at(currentStateNum);
        //printf("At state %s\n", currentState->getName().c_str());
        currentState->run();
        currentStateNum = currentState->nextState();
    }
}