#ifndef PROJECT_SYMBOLICVARSET_H
#define PROJECT_SYMBOLICVARSET_H

#include <unordered_map>
#include <stack>
#include <memory>

#include "SymbolicDouble.h"

typedef std::shared_ptr<SymbolicDouble> DoubleVarPointer;

class SymbolicVarSet
{
private:
    class TaggedStackUnion;
    SymbolicVarSet* parent;
    std::unordered_map<std::string, DoubleVarPointer> variables;
    std::stack<TaggedStackUnion> stack;

public:
    SymbolicVarSet(SymbolicVarSet* p = nullptr): parent(p){}
    DoubleVarPointer findVar(std::string name);
    DoubleVarPointer popVar();
    std::string popState();
};


#endif
