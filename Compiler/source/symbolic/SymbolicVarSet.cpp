#include "SymbolicVarSet.h"

using namespace std;

SymbolicDouble* SymbolicVarSet::findVar(string name)
{
    unordered_map<string, SymbolicDoublePointer>::const_iterator it = variables.find(name);
    if (it != variables.cend()) return it->second.get();

    else //must copy symbolic variable into this 'scope'
    {
        if (parent == nullptr) return nullptr;
        SymbolicDouble* oldSym = parent->findVar(name);
        if (oldSym == nullptr) return nullptr;
        variables[name] = make_unique<SymbolicDouble>(oldSym);
        return variables[name].get();
    }
}

SymbolicArray* SymbolicVarSet::findArray(string name)
{
    unordered_map<string, SymbolicArrayPointer>::const_iterator it = arrays.find(name);
    if (it != arrays.cend()) return it->second.get();
    else
    {
        if (parent == nullptr) return nullptr;
        SymbolicArray* oldSym = parent->findArray(name);
        if (oldSym == nullptr) return nullptr;
        SymbolicArrayPointer ap = oldSym->clone();
        SymbolicArray* newSym = ap.get();
        arrays[name] = move(ap);
        return newSym;
    }
}

bool SymbolicVarSet::isFeasable()
{
    for (auto& i : variables) if (!i.second->isFeasable()) return false;
    if (parent != nullptr && !parent->isFeasable()) throw std::runtime_error("shouldnt happen");
    return true;
}

void SymbolicVarSet::setLoopInit()
{
    for (auto& variable : variables) variable.second->loopInit();
    for (auto& array : arrays) array.second->loopInit();
    if (parent != nullptr) parent->setLoopInit();
}

void SymbolicVarSet::addVar(SymbolicDoublePointer newvar)
{
    variables[newvar->getName()] = move(newvar);
}

void SymbolicVarSet::addArray(const std::string& name, SymbolicArrayPointer p)
{
    arrays[name] = move(p);
}

//bool SymbolicVarSet::unionSVSSelective(S)

bool SymbolicVarSet::unionSVS(SymbolicVarSet* other)
{
    if (other == nullptr) throw std::runtime_error("cant union with nullptr");
    bool change = false;
    for (auto& pair : other->getAllVars())
    {
        SymbolicDouble* svp = findVar(pair.first);
        if (svp == nullptr)
        {
            change = true;
            variables[pair.first] = pair.second->clone();
        }
        else
        {
            if (svp->unionUpperBound(pair.second->getUpperBound())) change = true;
            if (svp->unionLowerBound(pair.second->getLowerBound())) change = true;
        }
    }

    for (auto& pair : other->getAllArrays())
    {
        SymbolicArray* sap = findArray(pair.first);
        if (sap == nullptr)
        {
            change = true;
            arrays[pair.first] = pair.second->clone();
        }
        else if (sap->unionArray(pair.second)) change = true;
    }
    return change;
}

std::unordered_map<std::string, SymbolicDouble*> SymbolicVarSet::getAllVars()
{
    std::unordered_map<std::string, SymbolicDouble*> toReturn = {};
    if (parent != nullptr) toReturn = parent->getAllVars();

    for (const auto& v : variables)
    {
        toReturn[v.first] = v.second.get();
    }

    return toReturn;
}

std::unordered_map<std::string, SymbolicArray*> SymbolicVarSet::getAllArrays()
{
    std::unordered_map<std::string, SymbolicArray*> toReturn = {};
    if (parent != nullptr) toReturn = parent->getAllArrays();

    for (const auto& v : arrays)
    {
        toReturn[v.first] = v.second.get();
    }

    return toReturn;
}

//iterator
const pair<const string, SymbolicDoublePointer>& SVSIterator::operator*()
{
    return *currentIt;
}

SVSIterator& SVSIterator::operator++()
{
    if (currentIt == currentSVS->endIterator) throw std::runtime_error("went too far");
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