//
// Created by abdul on 03/08/17.
//

//the exceptions thrown here should not have been caused by the users program

#include "SymbolicStack.h"

using namespace std;

SymbolicStack::SymbolicStack(Reporter& r) : reporter(r) {}
SymbolicStack::SymbolicStack(shared_ptr<SymbolicStack> p) : parent(move(p)), reporter(parent->reporter) {}

void SymbolicStack::setLoopInit()
{
    loopInit = true;
    for (auto& sm : currentStack) sm->setLoopInit();
}

void SymbolicStack::copyParent()
{
    if (parent == nullptr) throw runtime_error("Tried to copy parent of first frame");
    if (!currentStack.empty()) throw runtime_error("Cannot copy parent on a different branch");

    //need to duplicate SymbolicDoubles so we don't fiddle with other branches
    if (loopInit)
    {
        for (auto& sm : parent->currentStack)
        {
            auto newSM = sm->clone();
            newSM->setLoopInit();
            currentStack.push_back(move(newSM));
        }
    }
    else for (auto& sm : parent->currentStack) currentStack.emplace_back(sm->clone());
    parent = parent->parent;
}

void SymbolicStack::pushState(const string& pushedState)
{
    currentStack.emplace_back(make_unique<StateStackMember>(pushedState));
}

void SymbolicStack::pushVar(SymbolicVariable* pushedVar)
{
    currentStack.emplace_back(make_unique<SymVarStackMember>(pushedVar));
}

void SymbolicStack::pushDouble(double toPush)
{
    currentStack.emplace_back(make_unique<SymVarStackMember>(toPush));
}

void SymbolicStack::pushString(std::string toPush)
{
    currentStack.emplace_back(make_unique<SymVarStackMember>(toPush));
}

unique_ptr<StackMember> SymbolicStack::popMember()
{
    while (currentStack.empty())
    {
        if (parent == nullptr) throw runtime_error("Tried to pop empty stack");
        copyParent();
    }

    unique_ptr<StackMember> m = move(currentStack.back());
    currentStack.pop_back();
    return m;
}

string SymbolicStack::popState()
{
    unique_ptr<StackMember> popped = popMember();
    if (popped->getType() != SymbolicStackMemberType::STATE) throw runtime_error("Tried to jump to nonstate");
    return popped->getName();
}

string SymbolicStack::popString()
{
    unique_ptr<StackMember> popped = popMember();
    if (popped->getType() != SymbolicStackMemberType::VAR) throw runtime_error("wrong");
    return static_cast<SymVarStackMember*>(popped.get())->varptr->getConstString();
}

unique_ptr<SymbolicVariable> SymbolicStack::popVar()
{
    unique_ptr<StackMember> popped = popMember();
    if (popped->getType() != SymbolicStackMemberType::VAR) throw runtime_error("wrong");
    return move(static_cast<SymVarStackMember*>(popped.get())->varptr);
}

void SymbolicStack::copyStack(SymbolicStack* other)
{
    currentStack.clear();
    parent = other->parent;
    for (const auto& cptr: other->currentStack) currentStack.push_back(cptr->clone());
}

void SymbolicStack::unionStack(SymbolicStack* other)
{
    auto myIterator = currentStack.rbegin();
    SymbolicStack* currentCopyFrom = other;
    while (currentCopyFrom->currentStack.empty() && currentCopyFrom->parent != nullptr)
    {
        currentCopyFrom = currentCopyFrom->parent.get();
    }
    auto theirIterator = currentCopyFrom->currentStack.rbegin();

    while (theirIterator != currentCopyFrom->currentStack.rend()
            && !(theirIterator == currentCopyFrom->currentStack.rend() && currentCopyFrom->parent != nullptr))
    {
        if (myIterator == currentStack.rend()) //todo make this loop
        {
            vector<unique_ptr<StackMember>> newSt;
            for (auto tempIt = currentCopyFrom->currentStack.begin();;)
            {
                newSt.push_back((*tempIt)->clone());
                if (tempIt != theirIterator.base()) break;
                else ++tempIt;
            }
            for (auto& oldPtr : currentStack) newSt.push_back(move(oldPtr));
            currentStack = move(newSt);
            myIterator = currentStack.rend();
            theirIterator = currentCopyFrom->currentStack.rend();
        }
        if (theirIterator == currentCopyFrom->currentStack.rend() && currentCopyFrom->parent != nullptr)
        {
            currentCopyFrom = currentCopyFrom->parent.get();
            theirIterator = currentCopyFrom->currentStack.rbegin();
        }

        while (myIterator != currentStack.rend() && theirIterator != currentCopyFrom->currentStack.rend())
        {
            (*myIterator)->mergeSM(*theirIterator);
            ++myIterator; ++theirIterator;
        }
    }
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

SymbolicVariable* SymbolicStack::peekTopVar()
{
    while (currentStack.empty())
    {
        if (parent == nullptr) throw runtime_error("Tried to peek empty stack");
        copyParent();
    }
    StackMember* cb = currentStack.back().get();
    if (cb->getType() != SymbolicStackMemberType::VAR) throw runtime_error("Tried to pop nonvar as var");
    return static_cast<SymVarStackMember*>(cb)->varptr.get();
}

const string& SymbolicStack::peekTopName()
{
    while (currentStack.empty())
    {
        if (parent == nullptr) throw runtime_error("Tried to peek empty stack");
        copyParent();
    }
    currentStack.back()->getName();
}

SymbolicStackMemberType SymbolicStack::getTopType()
{
    while (currentStack.empty())
    {
        if (parent == nullptr) throw runtime_error("Tried to check top of empty stack");
        copyParent();
    }
    return currentStack.back()->getType();
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

string SymbolicStack::printStack()
{
    string toRet;
    if (parent != nullptr) toRet = parent->printStack();

    for (const auto& ptr : currentStack) toRet += ptr->diagString() + "\n";

    return toRet;
}