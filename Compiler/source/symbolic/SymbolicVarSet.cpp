#include "SymbolicVarSet.h"

using namespace std;

SymbolicVariablePointer SymbolicVarSet::findVar(std::string name)
{
    unordered_map<string, SymbolicVariablePointer>::const_iterator it = variables.find(name);
    if (it != variables.cend()) return it->second;

    else //must copy symbolic variable into this 'scope'
    {
        if (parent == nullptr) return SymbolicVariablePointer(); //null
        SymbolicVariablePointer oldSym = parent->findVar(name);
        if (oldSym.isNull()) return oldSym;
        SymbolicVariablePointer newSymPointer(oldSym);
        variables["name"] = newSymPointer;
        return newSymPointer;
    }
}

bool SymbolicVarSet::isFeasable()
{
    for (auto i : variables)
    {
        if (!i.second->isFeasable()) return false;
    }
    if (parent != nullptr && !parent->isFeasable())
    {
        return false; //should never happen
    }
    return true;
}