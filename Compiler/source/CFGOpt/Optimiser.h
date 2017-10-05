#ifndef PROJECT_OPTIMISE_H
#define PROJECT_OPTIMISE_H

#include "../compile/SymbolTable.h"
#include "../compile/Functions.h"
#include "../Command.h"

namespace Optimise
{
    void optimise(SymbolTable& symbolTable, ControlFlowGraph& controlFlowGraph);
    void collapseSmallStates(ControlFlowGraph& controlFlowGraph);
};


#endif
