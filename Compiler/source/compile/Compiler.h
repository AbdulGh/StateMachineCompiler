#ifndef PROJECT_COMPILER_H
#define PROJECT_COMPILER_H

#include <unordered_map>

#include "Lexer.h"
#include "Token.h"
#include "SymbolTable.h"
#include "Functions.h"
#include "ExpressionCodeGenerator.h"
#include "Functions.h"
#include "Reporter.h"
#include "../symbolic/SymbolicExecution.h"

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
    FunctionTable functionTable;
    ControlFlowGraph cfg;
    Reporter reporter;

    void error(std::string);
    void warning(std::string);
    Token nextToken();
    void findGlobalsAndMakeStates();
    std::shared_ptr<Identifier> findVariable(std::string);
    std::string quoteString(std::string& s);

    /*parsing*/
    void match(Type t);
    void body();
    bool statement(FunctionSymbol& fs); //returns true if the state has been ended
    Relations::Relop relop();
    void expression(FunctionSymbol& fs, const std::string& to);
    VariableType vtype();
    std::string ident();

    /*code generation*/
    VariableType genFunctionCall(FunctionSymbol&, std::shared_ptr<Identifier> toPtr = nullptr);
    void genIf(FunctionSymbol&);
    void genWhile(FunctionSymbol&);
    void ands(FunctionSymbol& fs, std::string success, std::string fail);
    void ors(FunctionSymbol& fs, std::string success, std::string fail);
    void condition(FunctionSymbol& fs, std::string success, std::string fail);

    friend class FunctionTable;
    friend class ControlFlowGraph;
};


#endif
