#ifndef PROJECT_COMPILER_H
#define PROJECT_COMPILER_H

#include <unordered_map>

#include "Lexer.h"
#include "Token.h"
#include "SymbolTable.h"
#include "FunctionCodeGen.h"
#include "ExpressionCodeGenerator.h"
#include "FunctionCodeGen.h"
#include "Reporter.h"

/* Implemented in Compiler.cpp, Parsing.cpp, CodeGen.cpp*/
class Compiler
{
    friend class ExpressionCodeGenerator;

public:
    Compiler(std::vector<Token>& st, std::string filename);
    void compile(std::stringstream&);

private:
    std::vector<Token> stream;
    std::vector<Token>::const_iterator tp;
    Token lookahead;
    SymbolTable symbolTable;
    std::unordered_map<std::string, FunctionPointer> functionTable;
    ControlFlowGraph cfg;
    Reporter reporter;

    void error(std::string);
    void warning(std::string);
    Token nextToken();
    void findGlobalsAndMakeFirstState();
    std::shared_ptr<Identifier> findVariable(std::string);
    FunctionPointer findFunction(std::string);
    std::string quoteString(std::string& s);

    /*parsing*/
    void match(Type t);
    void body();
    bool statement(FunctionPointer fs); //returns true if the state has been ended
    Relations::Relop relop();
    void expression(FunctionPointer fs, const std::string& to);
    VariableType vtype();
    std::string ident();

    /*code generation*/
    VariableType genFunctionCall(VariableType, FunctionPointer);
    void genIf(FunctionPointer);
    void genWhile(FunctionPointer);
    void ands(FunctionPointer fs, std::string success, std::string fail);
    void ors(FunctionPointer fs, std::string success, std::string fail);
    void condition(FunctionPointer fs, std::string success, std::string fail);
};


#endif
