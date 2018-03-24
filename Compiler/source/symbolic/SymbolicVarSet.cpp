#include "SymbolicVarSet.h"

using namespace std;

SymbolicVariable* SymbolicVarSet::findVar(string name)
{
    unordered_map<string, SymbolicVariablePointer>::const_iterator it = variables.find(name);
    if (it != variables.cend()) return it->second.get();

    else //must copy symbolic variable into this 'scope'
    {
        if (parent == nullptr) return nullptr;
        SymbolicVariable* oldSym = parent->findVar(name);
        if (oldSym == nullptr) return nullptr;
        if (oldSym->getType() == DOUBLE)
        {
            variables[name] = make_unique<SymbolicDouble>(oldSym);
            return variables[name].get();
        }
        else if (oldSym->getType() == STRING)
        {
            variables[name] = make_unique<SymbolicString>(oldSym);
            return variables[name].get();
        }
        else throw runtime_error("Bad type found");
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
    if (parent != nullptr && !parent->isFeasable()) throw "shouldnt happen";
    return true;
}

void SymbolicVarSet::setLoopInit()
{
    for (auto& variable : variables) variable.second->loopInit();
    if (parent != nullptr) parent->setLoopInit();
}

void SymbolicVarSet::addVar(SymbolicVariablePointer newvar)
{
    variables.insert({newvar->getName(), move(newvar)});
}

void SymbolicVarSet::addArray(const std::string& name, SymbolicArrayPointer p)
{
    arrays[name] = move(p);
}

bool SymbolicVarSet::unionSVS(SymbolicVarSet* other)
{
    if (other == nullptr) throw "cant union with nullptr";
    bool change = false;
    for (auto& pair : other->getAllVars())
    {
        SymbolicVariable* svp = findVar(pair.first);
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
    return change;
}

vector<pair<const string, SymbolicVariable*>> SymbolicVarSet::getAllVars()
{
    vector<pair<const string, SymbolicVariable*>> toReturn = {};
    if (parent != nullptr) toReturn = parent->getAllVars();

    for (const auto& v : variables)
    {
        toReturn.emplace_back(pair<const string, SymbolicVariable*>(v.first, v.second.get()));
    }

    return toReturn;
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