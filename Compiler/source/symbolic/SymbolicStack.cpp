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
    if (!currentStack.empty()) throw runtime_error("Cannot copy parent on a different branch");

    //need to duplicate SymbolicDoubles so we don't fiddle with other branches
    for (StackMember& sm : parent->currentStack) currentStack.emplace_back(StackMember(sm)); //copies
    parent = parent->parent;
}

void SymbolicStack::push(const string& pushedState)
{
    currentStack.emplace_back(StackMember(pushedState));
}

void SymbolicStack::push(SymbolicVariable* pushedVar)
{
    currentStack.emplace_back(StackMember(pushedVar));
}

StackMember SymbolicStack::popMember()
{
    while (currentStack.empty())
    {
        if (parent == nullptr) throw runtime_error("Tried to pop empty stack");
        copyParent();
    }

    StackMember m = move(currentStack.back());
    currentStack.pop_back();
    return m;
}

string SymbolicStack::popState()
{
    StackMember popped = popMember();
    if (popped.type != SymbolicStackMemberType::STATE) throw runtime_error("Tried to jump to var");
    return popped.statename;
}

void SymbolicStack::pop()
{
    while (currentStack.empty())
    {
        if (parent == nullptr) throw runtime_error("Tried to pop empty stack");
        copyParent();
    }
    currentStack.pop_back();
}

unique_ptr<SymbolicVariable> SymbolicStack::popVar()
{
    StackMember popped = popMember();
    if (popped.type != SymbolicStackMemberType::VAR) throw runtime_error("Tried to pop state as var");
    return move(popped.varptr);
}

SymbolicVariable* SymbolicStack::peekTopVar()
{
    while (currentStack.empty())
    {
        if (parent == nullptr) throw runtime_error("Tried to peek empty stack");
        copyParent();
    }
    StackMember& cb = currentStack.back();
    if (cb.type != SymbolicStackMemberType::VAR) throw runtime_error("Tried to pop state as var");
    return cb.varptr.get();
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
    if (currentStack.empty())
    {
        if (parent == nullptr) return true;
        return parent->isEmpty();
    }
    return false;
}

