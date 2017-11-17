#ifndef FSM_H
#define FSM_H

#include <vector>
#include <string>
#include <stack>

#include "Variable.h"
#include "State.h"
#include "Command.h"

class FSM
{
    friend class State;
    template<class T> friend class PushCommand;
    friend class PopCommand;
    friend class ReturnCommand;
    template<class T> friend class JumpOnComparisonCommand;
private:
    int state;
    std::vector<std::unique_ptr<State>> states;
    std::stack<Variable::TaggedDataUnion> sharedStack;
public:
    void setStates(std::vector<std::unique_ptr<State>>);

    void run();
};


#endif
