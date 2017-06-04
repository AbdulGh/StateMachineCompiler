#ifndef FSM_H
#define FSM_H

#include <vector>
#include <string>
#include <stack>

#include "Variable.h"


class State;
class PushVarCommand;
class PopVarCommand;

class FSM
{
    friend class State;
    friend class PushVarCommand;
    friend class PopVarCommand;
private:
    int state;
    std::vector<std::shared_ptr<State>> states;
    std::stack<std::shared_ptr<Variable>> sharedStack;
public:
    void setStates(std::vector<std::shared_ptr<State>>);

    void run();
};


#endif
