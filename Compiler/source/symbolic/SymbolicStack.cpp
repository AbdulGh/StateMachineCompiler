//
// Created by abdul on 03/08/17.
//

//the exceptions thrown here should not have been caused by the users program

#include "SymbolicStack.h"

using namespace std;

SymbolicStack::SymbolicStack(shared_ptr<SymbolicStack> parent)
{
    this->parent = parent;
}

void SymbolicStack::copyParent()
{
    if (parent == nullptr) throw runtime_error("Tried to copy parent of first frame");
    if (currentStack.size() != 0) throw runtime_error("Cannot copy parent on a different branch");

    //need to duplicate SymbolicDoubles so we don't fiddle with other branches
    for (StackMember sm : parent->currentStack)
    {
        currentStack.push_back(sm);
    }

    parent = parent->parent;
}

void SymbolicStack::push(string pushedState)
{
    currentStack.push_back(StackMember(pushedState));
}

//void SymbolicStack::push(unique_ptr<SymbolicVariable> pushedVar)
//{
//    currentStack.push_back(StackMember(move(pushedVar)));
//}

void SymbolicStack::push(SymbolicVariable* pushedVar)
{
    currentStack.push_back(StackMember(pushedVar));
}

StackMember SymbolicStack::popMember()
{
    while (currentStack.size() == 0)
    {
        if (parent == nullptr) throw runtime_error("Tried to pop empty stack");
        copyParent();
    }

    StackMember m = currentStack.back();
    currentStack.pop_back();
    return move(m);
}

string SymbolicStack::popState()
{
    StackMember popped = popMember();
    if (popped.type != SymbolicStackMemberType::STATE) throw runtime_error("Tried to jump to var");
    return popped.statename;
}

unique_ptr<SymbolicVariable> SymbolicStack::popVar()
{
    StackMember popped = popMember();
    if (popped.type != SymbolicStackMemberType::VAR) throw runtime_error("Tried to pop state as var");
    return move(popped.varptr);
}

SymbolicStackMemberType SymbolicStack::getTopType()
{
    while (currentStack.empty())
    {
        if (parent == nullptr) throw runtime_error("Tried to check top of empty stack");
        copyParent();
    }

    return currentStack.back().type;
}

bool SymbolicStack::isEmpty()
{
    while (currentStack.empty())
    {
        if (parent == nullptr) return true;
        copyParent();
    }
    return false;
}

