#ifndef PROJECT_COMPILER_H
#define PROJECT_COMPILER_H

#include <unordered_map>

#include "Lexer.h"
#include "Token.h"
#include "SymbolTable.h"
#include "FunctionSymbol.h"
#include "ExpressionCodeGenerator.h"
#include "FunctionSymbol.h"

typedef std::shared_ptr<FunctionSymbol> FunctionPointer;

class Compiler
{
    friend class ExpressionCodeGenerator;

public:
    Compiler(std::vector<Token>& st);
    void compile(std::stringstream&);

private:
    std::vector<Token> stream;
    std::vector<Token>::const_iterator tp;
    Token lookahead;
    SymbolTable symbolTable;
    std::unordered_map<std::string, FunctionPointer> functionTable;

    void error(std::string);
    Token nextToken();
    void findFunctionsAndMakeFirstStates(std::stringstream&);
    std::shared_ptr<Identifier> findVariable(std::string);
    FunctionPointer findFunction(std::string);

    /*parsing*/
    void match(Type t);
    void body();
    bool statement(FunctionPointer fs); //returns true if the state has been ended
    void ands(FunctionPointer fs);
    void ors(FunctionPointer fs);
    void condition(FunctionPointer fs);
    void relop();
    void expression(FunctionPointer fs, const std::string& to);
    VariableType vtype();
    std::string ident();

    /*code generation*/
    VariableType genFunctionCall(VariableType, FunctionPointer);
};


#endif
