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
    std::unique_ptr<SymbolicVariable> varptr;
    char* statename = nullptr;

    StackMember(const std::string& state)
    {
        type = STATE;
        statename = new char[state.length() + 1];
        std::copy(state.begin(), state.end(), statename);
        statename[state.size()] = '\0';
    }
    StackMember(SymbolicVariable* toPush)
    {
        type = VAR;
        if (toPush->getType() == DOUBLE) varptr = std::make_unique<SymbolicDouble>(toPush);
        else if (toPush->getType() == STRING) varptr = std::make_unique<SymbolicString>(toPush);
        else throw "bad dtype";
    }
    StackMember(const StackMember& sm)
    {
        type = sm.type;
        if (type == STATE)
        {
            std::string state = sm.statename; //todo quick copy directly
            statename = new char[state.length() + 1];
            std::copy(state.begin(), state.end(), statename);
            statename[state.size()] = '\0';
        }
        else
        {
            if (sm.varptr->getType() == DOUBLE) varptr = std::make_unique<SymbolicDouble>(sm.varptr.get());
            else if (sm.varptr->getType() == STRING) varptr = std::make_unique<SymbolicString>(sm.varptr.get());
            else throw "bad dtype";
        }
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
    std::vector<StackMember> currentStack;

    void copyParent();
    StackMember popMember();
public:
    SymbolicStack(std::shared_ptr<SymbolicStack> parent = nullptr);
    //void push(std::unique_ptr<SymbolicVariable> pushedVar);
    void push(SymbolicVariable* pushedVar);
    void push(const std::string& pushedState);
    std::unique_ptr<SymbolicVariable> popVar();
    std::string popState();
    bool isEmpty();
    SymbolicStackMemberType getTopType();
};


#endif //PROJECT_SYMBOLICSTACK_H
