#include "FunctionSymbol.h"

using namespace std;

FunctionSymbol::FunctionSymbol(VariableType rt, std::vector<VariableType> types, std::string in):
    returnType(rt), paramTypes(types), ident(in), currentStates(0){}

bool FunctionSymbol::checkTypes(std::vector<VariableType>& potential)
{
    return ((potential.size() == paramTypes.size()) && paramTypes == potential);
}

const string FunctionSymbol::genNewStateName()
{
    return "F_" + ident + "_" + to_string(currentStates++);
}

const string& FunctionSymbol::getIdentifier() const
{
    return ident;
}

bool FunctionSymbol::isOfType(VariableType c)
{
    return (c == returnType);
}

string FunctionSymbol::getSource()
{
    return currentSource.str();
}

void FunctionSymbol::emit(string s)
{
    currentSource << s;
}

VariableType FunctionSymbol::getReturnType() const
{
    return returnType;
}
