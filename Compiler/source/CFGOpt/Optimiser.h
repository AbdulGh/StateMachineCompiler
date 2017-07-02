#ifndef PROJECT_OPTIMISE_H
#define PROJECT_OPTIMISE_H

#include "../compile/SymbolTable.h"
#include "../compile/FunctionCodeGen.h"
#include "../Command.h"

typedef std::shared_ptr<FunctionCodeGen> FunctionPointer;

class Optimiser
{
private:
    SymbolTable& symbolTable;
    std::unordered_map<std::string, FunctionPointer>& functionTable;

    void collapseSmallStates();
    void replaceJumps(std::string sName, std::string replaceWith);

    void propogateConstants();
public:
    Optimiser(SymbolTable& s, std::unordered_map<std::string, FunctionPointer>& f):
            symbolTable(s), functionTable(f) {}
    void optimise();
};


#endif
