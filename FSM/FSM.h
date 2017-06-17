#ifndef FSM_H
#define FSM_H

#include <vector>
#include <string>
#include <stack>

#include "Variable.h"


class State;
template<class T> class PushCommand;
class PopCommand;
class JumpTopCommand;
template<class T> class JumpOnComparisonCommand;

class FSM
{
    friend class State;
    template<class T> friend class PushCommand;
    friend class PopCommand;
    friend class JumpTopCommand;
    template<class T> friend class JumpOnComparisonCommand;
private:
    int state;
    std::vector<std::shared_ptr<State>> states;
    std::stack<Variable::TaggedDataUnion> sharedStack;
public:
    void setStates(std::vector<std::shared_ptr<State>>);

    void run();
};


#endif
