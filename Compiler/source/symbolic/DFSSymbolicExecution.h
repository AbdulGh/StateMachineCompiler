#ifndef PROJECT_DFSSYMBOLICEXECUTION_H
#define PROJECT_DFSSYMBOLICEXECUTION_H

#include <unordered_map>
#include <stack>

#include "../CFGOpt/CFG.h"
#include "SymbolicDouble.h"

typedef std::shared_ptr<SymbolicDouble> DoubleVarPointer;

class DFSSymbolicExecution
{
private:
    std::unordered_map<std::string, int> timesVisited;
    std::stack<SymbolicDouble> varstack;
    ControlFlowGraph& cfg;
    SymbolTable& sTable;

public:
    DFSSymbolicExecution(ControlFlowGraph& cfg, SymbolTable& sTable):
        cfg(cfg), sTable(sTable) {};
    void removeUnreachableStates();
};


#endif