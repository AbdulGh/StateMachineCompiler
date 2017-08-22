//
// Created by abdul on 03/08/17.
//

#ifndef PROJECT_SYMBOLICSTACK_H
#define PROJECT_SYMBOLICSTACK_H

#include <vector>

#include "SymbolicVariables.h"

enum SymbolicStackMemberType {STATE, VAR};

struct StackMember
{
    SymbolicStackMemberType type;
    union
    {
        std::shared_ptr<SymbolicVariable> varptr;
        char* statename;
    };

    StackMember(std::string state)
    {
        type = STATE;
        statename = new char[state.length() + 1];
        std::copy(state.begin(), state.end(), statename);
        statename[state.size()] = '\0';
    }
    StackMember(std::shared_ptr<SymbolicVariable> toPush)
    {
        type = VAR;
        if (toPush->getType() == DOUBLE) varptr = std::make_shared<SymbolicDouble>(toPush);
        else varptr = std::make_shared<SymbolicString>(toPush);
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
        else
        {
            if (sm.varptr->getType() == DOUBLE) varptr = std::make_shared<SymbolicDouble>(sm.varptr);
            else varptr = std::make_shared<SymbolicString>(sm.varptr);
        }
    }
    StackMember(std::shared_ptr<StackMember> other): StackMember(*other.get()) {}

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
    std::vector<std::shared_ptr<StackMember>> currentStack;

    void copyParent();
    std::shared_ptr<StackMember> popMember();
public:
    SymbolicStack(std::shared_ptr<SymbolicStack> parent = nullptr);
    void push(std::shared_ptr<SymbolicVariable> pushedVar);
    void push(std::string pushedState);
    std::shared_ptr<SymbolicVariable> popVar();
    std::string popState();
    bool isEmpty();
    SymbolicStackMemberType getTopType();
};


#endif //PROJECT_SYMBOLICSTACK_H
