#ifndef PROJECT_SYMBOLICVARSET_H
#define PROJECT_SYMBOLICVARSET_H

#include <unordered_map>
#include <stack>
#include <memory>

#include "SymbolicVariables.h"


typedef std::shared_ptr<SymbolicVariable> SymbolicVariablePointer;

class SymbolicVarSet
{
private:
    std::shared_ptr<SymbolicVarSet> parent;
    std::unordered_map<std::string, SymbolicVariablePointer> variables;

public:
    SymbolicVarSet(std::shared_ptr<SymbolicVarSet> p = nullptr): parent(p){}
    SymbolicVariablePointer findVar(std::string name);
    void defineVar(SymbolicVariablePointer newvar);
    bool isFeasable();
};


#endif
