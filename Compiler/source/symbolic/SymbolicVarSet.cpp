#include "SymbolicVarSet.h"

using namespace std;

SymbolicVariablePointer SymbolicVarSet::findVar(string name)
{
    unordered_map<string, SymbolicVariablePointer>::const_iterator it = variables.find(name);
    if (it != variables.cend()) return it->second;

    else //must copy symbolic variable into this 'scope'
    {
        if (parent == nullptr) return nullptr;
        SymbolicVariablePointer oldSym = parent->findVar(name);
        if (oldSym == nullptr) return nullptr;
        if (oldSym->getType() == DOUBLE)
        {
            auto newPtr = make_shared<SymbolicDouble>(oldSym);
            variables.insert({name, newPtr});
            return newPtr;
        }
        else if (oldSym->getType() == STRING)
        {
            auto newPtr = make_shared<SymbolicString>(oldSym);
            variables.insert({name, newPtr});
            return newPtr;
        }
        else throw runtime_error("Bad type found");
    }
}

bool SymbolicVarSet::isFeasable()
{
    for (auto i : variables)
    {
        if (!i.second->isFeasable()) return false;
    }
    if (parent != nullptr && !parent->isFeasable()) throw "shouldnt happen";
    return true;
}

void SymbolicVarSet::setLoopInit()
{
    for (auto variable : variables) variable.second->loopInit();
    if (parent != nullptr) parent->setLoopInit();
}

void SymbolicVarSet::defineVar(SymbolicVariablePointer newvar)
{
    variables[newvar->getName()] = newvar;
}

void SymbolicVarSet::unionSVS(std::shared_ptr<SymbolicVarSet> other)
{
    if (other == nullptr) throw "cant union with nullptr";

    for (auto pair : other->variables)
    {
        SymbolicVariablePointer svp = findVar(pair.first);
        if (svp == nullptr) variables[pair.first] = pair.second->clone();
        else
        {
            svp->unionUpperBound(pair.second->getUpperBound());
            svp->unionLowerBound(pair.second->getLowerBound());
        }
    }
    if (other->parent != nullptr) unionSVS(other->parent);
}

//iterator
const pair<const string, SymbolicVariablePointer>& SVSIterator::operator*()
{
    return *currentIt;
}

SVSIterator& SVSIterator::operator++()
{

    if (currentIt == currentSVS->endIterator) throw "went too far";
    ++currentIt;
    if (currentIt == currentSVS->variables.cend() && currentSVS->parent != nullptr)
    {
        currentSVS = currentSVS->parent.get();
        currentIt = currentSVS->variables.cbegin();
    }
    return *this;
}

bool SVSIterator::operator!=(const SVSIterator& other)
{
    return (other.currentSVS != currentSVS || other.currentIt != currentIt);
}