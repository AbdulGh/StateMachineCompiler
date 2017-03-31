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
    int checkState(std::string);

public:
    FSM(std::string);
    void run();
};


#endif
