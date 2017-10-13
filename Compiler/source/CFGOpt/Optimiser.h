#ifndef PROJECT_OPTIMISE_H
#define PROJECT_OPTIMISE_H

#include "../compile/SymbolTable.h"
#include "../compile/Functions.h"
#include "../Command.h"

namespace Optimise
{
    void optimise(SymbolTable& symbolTable, FunctionTable& functionTable, ControlFlowGraph& controlFlowGraph);
    void collapseSmallStates(ControlFlowGraph& controlFlowGraph, FunctionTable& functionTable);
    void destroyBinaryCFGStructure(ControlFlowGraph& controlFlowGraph);
};


#endif
