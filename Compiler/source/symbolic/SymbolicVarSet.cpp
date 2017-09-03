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
            shared_ptr<SymbolicDouble> oldDouble = static_pointer_cast<SymbolicDouble>(oldSym);
            shared_ptr<SymbolicVariable> newSymPointer = make_shared<SymbolicDouble>(oldDouble); //copies
            variables["name"] = newSymPointer;
            return newSymPointer;
        }
        else if (oldSym->getType() == STRING)
        {
            shared_ptr<SymbolicString> oldString = static_pointer_cast<SymbolicString>(oldSym);
            shared_ptr<SymbolicVariable> newSymPointer = make_shared<SymbolicString>(oldString);
            variables["name"] = newSymPointer;
            return newSymPointer;
        }
        else throw runtime_error("Bad type found");
    }
}

/*
template <typename T>
shared_ptr<SymbolicVariableTemplate<T>> SymbolicVarSet::findVar(std::string name)
{
    return static_pointer_cast<SymbolicVariableTemplate<T>>(findVar(name));
}
shared_ptr<SymbolicVariableTemplate<string>> SymbolicVarSet::findVar<string>(std::string name);
shared_ptr<SymbolicVariableTemplate<double>> SymbolicVarSet::findVarOfType<double>(std::string name);*/

bool SymbolicVarSet::isFeasable()
{
    for (auto i : variables)
    {
        if (!i.second->isFeasable())
        {
            return false;
        }
    }
    if (parent != nullptr && !parent->isFeasable())
    {
        return false; //should never happen
    }
    return true;
}

void SymbolicVarSet::defineVar(SymbolicVariablePointer newvar)
{
    variables[newvar->getName()] = newvar;
}