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
    for (shared_ptr<StackMember>& sm : parent->currentStack)
    {
        currentStack.push_back(make_shared<StackMember>(sm));
    }

    parent = parent->parent;
}

void SymbolicStack::push(string pushedState)
{
    currentStack.push_back(make_shared<StackMember>(pushedState));
}

void SymbolicStack::push(shared_ptr<SymbolicVariable> pushedVar)
{
    currentStack.push_back(make_shared<StackMember>(pushedVar));
}

shared_ptr<StackMember> SymbolicStack::popMember()
{
    while (currentStack.size() == 0)
    {
        if (parent == nullptr) throw runtime_error("Tried to pop empty stack");
        copyParent();
    }

    shared_ptr<StackMember> m = move(currentStack.back());
    currentStack.pop_back();
    return m;
}

string SymbolicStack::popState()
{
    shared_ptr<StackMember> popped = popMember();
    if (popped->type != SymbolicStackMemberType::STATE) throw runtime_error("Tried to jump to var");
    return popped->statename;
}

shared_ptr<SymbolicVariable> SymbolicStack::popVar()
{
    shared_ptr<StackMember> popped = popMember();
    if (popped->type != SymbolicStackMemberType::VAR) throw runtime_error("Tried to pop state as var");
    return popped->varptr;
}

SymbolicStackMemberType SymbolicStack::getTopType()
{
    while (currentStack.empty())
    {
        if (parent == nullptr) throw runtime_error("Tried to check top of empty stack");
        copyParent();
    }

    return currentStack.back()->type;
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

