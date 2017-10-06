#include "Functions.h"
#include "Compiler.h"
using namespace std;

FunctionSymbol* FunctionTable::getFunction(const std::string& funcName)
{
    auto it = functionTable.find(funcName);
    if (it == functionTable.cend()) parent.error("Undefined function '" + funcName + "'");
    return it->second.get();
}

FunctionSymbol* FunctionTable::addFunction(VariableType returnType, std::vector<VariableType>& types, std::string& ident)
{
    string prefix = "F" + to_string(functionTable.size()) + "_" + ident + "_";
    functionTable[ident] = std::make_unique<FunctionSymbol>(returnType, types, prefix, parent.cfg);
    return functionTable[ident].get();
}

bool FunctionTable::containsFunction(const std::string& funcName)
{
    return (functionTable.find(funcName) != functionTable.cend());
}

//assumes it's in the right format
FunctionSymbol* FunctionTable::getParentFunc(std::string stateName)
{
    if (stateName.empty()) throw "not good";
    unsigned long underscore = 0;
    while (stateName[underscore] != '_') ++underscore;
    stateName.erase(0, underscore+1);

    underscore = stateName.size() - 1;
    while (stateName[underscore] != '_') --underscore;
    stateName.erase(underscore, stateName.size() - 1);

    return getFunction(stateName);
}