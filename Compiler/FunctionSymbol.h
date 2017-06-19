#ifndef PROJECT_FUNCTIONSYMBOL_H
#define PROJECT_FUNCTIONSYMBOL_H

#include <vector>
#include <sstream>

#include "Token.h"


class FunctionSymbol
{
private:
    VariableType returnType;
    std::vector<VariableType> paramTypes;
    std::stringstream currentSource;
    int currentStates;
    std::string ident;
public:
    FunctionSymbol(VariableType returnType, std::vector<VariableType> types, std::string ident);
    const std::string genNewStateName();
    const std::string& getIdentifier() const;
    bool checkTypes(std::vector<VariableType>& potential);
    bool isOfType(VariableType c);
    void emit(std::string);
    std::string getSource();
    VariableType getReturnType() const;
};


#endif
