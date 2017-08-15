//
// Created by abdul on 03/08/17.
//

#ifndef PROJECT_SYMBOLICSTACK_H
#define PROJECT_SYMBOLICSTACK_H

#include <vector>

#include "SymbolicVariables.h"

struct StackMember
{
    enum {STATE, VAR} type;
    union
    {
        std::shared_ptr<SymbolicDouble> varptr;
        char* statename;
    };

    StackMember(std::string state)
    {
        type = STATE;
        statename = new char[state.length() + 1];
        std::copy(state.begin(), state.end(), statename);
        statename[state.size()] = '\0';
    }
    StackMember(std::shared_ptr<SymbolicDouble> toPush)
    {
        type = VAR;
        varptr = std::shared_ptr<SymbolicDouble>(new SymbolicDouble(toPush));
    }
    StackMember(const StackMember& sm)
    {
        type = sm.type;
        if (type == STATE)
        {
            std::string state = sm.statename;
            statename = new char[state.length() + 1];
            std::copy(state.begin(), state.end(), statename);
            statename[state.size()] = '\0';
        }
        else varptr = std::shared_ptr<SymbolicDouble>(new SymbolicDouble(sm.varptr));
    }

    ~StackMember()
    {
        if (type == STATE) delete[] statename;
        else
        {
            varptr.reset();
            varptr = nullptr;
        }
    }
};

class SymbolicStack
{
private:
    std::shared_ptr<SymbolicStack> parent;
    std::vector<std::shared_ptr<StackMember>> currentStack; //no excessive copying

    void copyParent();
    std::shared_ptr<StackMember> popMember();
public:
    SymbolicStack(std::shared_ptr<SymbolicStack> parent = nullptr);
    void push(std::shared_ptr<SymbolicDouble> pushedVar);
    void push(std::string pushedState);
    std::shared_ptr<SymbolicDouble> popVar();
    std::string popState();
};


#endif //PROJECT_SYMBOLICSTACK_H
