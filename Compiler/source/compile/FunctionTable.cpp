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
    unique_ptr<FunctionSymbol> newFunc = make_unique<FunctionSymbol>(returnType, types, ident, prefix, parent.cfg);
    FunctionSymbol* toReturn = newFunc.get();
    functionTable[ident] = move(newFunc);
    return toReturn;
}

bool FunctionTable::containsFunction(const std::string& funcName)
{
    return (functionTable.find(funcName) != functionTable.cend());
}

/*
bool FunctionTable::containsFunctionPrefix(const std::string& funcName)
{
    return (functionTable.find(removeUnderscoreWrappers(funcName)) != functionTable.cend());
}*/

//assumes it's in the right format
std::string FunctionTable::removeUnderscoreWrappers(std::string underscored)
{
    if (underscored.empty()) throw "not good";
    unsigned long underscore = 0;
    while (underscored[underscore] != '_') ++underscore;
    underscored.erase(0, underscore+1);

    underscore = underscored.size() - 1;
    while (underscored[underscore] != '_') --underscore;
    underscored.erase(underscore, underscored.size() - 1);
    return underscored;
}

FunctionSymbol* FunctionTable::getParentFunc(std::string stateName)
{
    stateName = removeUnderscoreWrappers(stateName);
    return getFunction(stateName);
}

void FunctionTable::removeFunction(const std::string& ident)
{
    //string ident = removeUnderscoreWrappers(bye);
    auto it = functionTable.find(ident);
    (*it->second).clearFunctionCalls();
    if (it == functionTable.cend()) throw "cant find function to remove";
    functionTable.erase(it);
}