#ifndef PROJECT_SYMBOLICVARSET_H
#define PROJECT_SYMBOLICVARSET_H

#include <unordered_map>
#include <stack>
#include <memory>

#include "SymbolicVariable.h"

typedef std::shared_ptr<SymbolicVariable> SymbolicVarPointer;

class SymbolicVarSet
{
private:
    class TaggedStackUnion;
    SymbolicVarSet* parent;
    std::unordered_map<std::string, SymbolicVarPointer> variables;
    std::stack<TaggedStackUnion> stack;

public:
    SymbolicVarSet(SymbolicVarSet* p = nullptr): parent(p){}
    SymbolicVarPointer findVar(std::string name);
    SymbolicVarPointer popVar();
    std::string popState();
};


#endif
