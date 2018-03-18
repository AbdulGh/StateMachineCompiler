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

enum class AccessType;
class VarWrapper;

//Implemented in Compiler.cpp, Parsing.cpp, CodeGen.cpp
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
    Identifier* findVariable(VarWrapper* varGetter, VariableType* vtype = nullptr);
    std::string quoteString(std::string& s);

    //parsing
    void match(Type t);
    void body();
    bool statement(FunctionSymbol* fs); //returns true if the state has been ended
    Relations::Relop relop();
    void expression(FunctionSymbol* fs, std::unique_ptr<VarSetter> to);
    VariableType vtype(unsigned int* = nullptr);
    std::unique_ptr<VarGetter> identGetter(AccessType* at = nullptr);
    std::unique_ptr<VarSetter> identSetter(AccessType* at = nullptr);
    std::string identPlain();

    //code generation
    VariableType genFunctionCall(FunctionSymbol*, VariableType expectedType, std::unique_ptr<VarSetter> vs = nullptr);
    void genIf(FunctionSymbol*);
    void genWhile(FunctionSymbol*);
    void ands(FunctionSymbol* fs, std::string success, std::string fail);
    void ors(FunctionSymbol* fs, std::string success, std::string fail);
    void condition(FunctionSymbol* fs, std::string success, std::string fail);

    friend class FunctionTable;
    friend class ControlFlowGraph;
};


#endif
