#ifndef PROJECT_DFSSYMBOLICEXECUTION_H
#define PROJECT_DFSSYMBOLICEXECUTION_H

#include <unordered_map>
#include <stack>

#include "../CFGOpt/CFG.h"
#include "SymbolicDouble.h"
#include "../compile/Token.h" //relop

typedef std::shared_ptr<SymbolicDouble> DoubleVarPointer;

class DFSSymbolicExecution
{
private:
    struct Condition
    {
        std::string l;
        Relop c;
        std::string r;

        Condition(std::string lhs, Relop comp, std::string rhs):
                l(lhs), r(rhs), c(comp) {}
    };

    std::unordered_map<std::string, int> timesVisited;

    std::stack<StackMember> currentStack;
    std::stack<Condition> pathCondition;
    ControlFlowGraph& cfg;
    SymbolTable& sTable;

public:
    DFSSymbolicExecution(ControlFlowGraph& cfg, SymbolTable& sTable):
        cfg(cfg), sTable(sTable) {};
    void search();
    void removeUnreachableStates();
};


#endif