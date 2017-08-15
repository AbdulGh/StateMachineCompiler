//
// Created by abdul on 03/08/17.
//

//the exceptions thrown here should not have been caused by the users program

#include "SymbolicStack.h"

using namespace std;

SymbolicStack::SymbolicStack(std::shared_ptr<SymbolicStack> parent)
{
    this->parent = parent;
}

void SymbolicStack::copyParent()
{
    if (parent == nullptr) throw runtime_error("Tried to copy parent of first frame");
    if (currentStack.size() != 0) throw runtime_error("Cannot copy parent on a different branch");

    //need to duplicate SymbolicDoubles so we don't fiddle with other branches
    for (std::shared_ptr<StackMember>& sm : parent->currentStack)
    {
        currentStack.push_back(sm); //is this okay?
    }

    parent = parent->parent;
}

void SymbolicStack::push(std::string pushedState)
{
    currentStack.push_back(shared_ptr<StackMember>(new StackMember(pushedState)));
}

void SymbolicStack::push(std::shared_ptr<SymbolicDouble> pushedVar)
{
    currentStack.push_back(shared_ptr<StackMember>(new StackMember(pushedVar)));
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
    if (popped->type != StackMember::STATE) throw runtime_error("Tried to jump to var");
    return popped->statename;
}

shared_ptr<SymbolicDouble> SymbolicStack::popVar()
{
    shared_ptr<StackMember> popped = popMember();
    if (popped->type != StackMember::VAR) throw runtime_error("Tried to pop state as var");
    return popped->varptr;
}

