#ifndef FSM_H
#define FSM_H

#include <vector>
#include <string>

#include "State.h"

class FSM
{
private:
    int state;
    std::vector<std::shared_ptr<State>> states;

public:
    FSM(std::vector<std::shared_ptr<State>> st);
    void run();
};


#endif
