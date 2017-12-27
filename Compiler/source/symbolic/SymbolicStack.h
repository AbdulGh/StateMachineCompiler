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
    std::string statename;

    explicit StackMember(const std::string& state)
    {
        type = STATE;
        statename = std::string(state);
    }

    explicit StackMember(SymbolicVariable* toPush)
    {
        type = VAR;
        if (toPush->getType() == DOUBLE) varptr = std::make_unique<SymbolicDouble>(toPush);
        else if (toPush->getType() == STRING) varptr = std::make_unique<SymbolicString>(toPush);
        else throw "bad dtype";
    }

    StackMember(const StackMember& sm)
    {
        type = sm.type;
        if (type == STATE)  statename = std::string(sm.statename);
        else
        {
            if (sm.varptr->getType() == DOUBLE) varptr = std::make_unique<SymbolicDouble>(sm.varptr.get());
            else if (sm.varptr->getType() == STRING) varptr = std::make_unique<SymbolicString>(sm.varptr.get());
            else throw "bad dtype";
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
    SymbolicStack(const SymbolicStack&) = delete;
    //void push(std::unique_ptr<SymbolicVariable> pushedVar);
    void push(SymbolicVariable* pushedVar);
    void push(const std::string& pushedState);
    std::unique_ptr<SymbolicVariable> popVar();
    SymbolicVariable* peekTopVar();
    const std::string& peekTopName();
    std::string popState();
    void pop();
    bool isEmpty();
    SymbolicStackMemberType getTopType();
};


#endif //PROJECT_SYMBOLICSTACK_H
