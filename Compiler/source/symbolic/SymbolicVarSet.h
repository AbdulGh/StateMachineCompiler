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
    SymbolicVarSet(std::shared_ptr<SymbolicVarSet> p = nullptr): parent(move(p)){}
    SymbolicVariablePointer findVar(std::string name);
    const std::unordered_map<std::string, SymbolicVariablePointer>& getVars() const {return variables;}
    void defineVar(SymbolicVariablePointer newvar);
    void unionSVS(std::shared_ptr<SymbolicVarSet> other);
    bool isFeasable();
};


#endif
