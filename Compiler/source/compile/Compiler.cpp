//
// Created by abdul on 10/08/17.
//
#include <iostream>
#include <algorithm>

#include "Compiler.h"
#include "../CFGOpt/Optimiser.h"
#include "../CFGOpt/LengTarj.h"
#include "../CFGOpt/Loop.h"

using namespace std;

Compiler::Compiler(vector<Token>& st, string auxFileName): stream(st), reporter(move(auxFileName)),
                                                           functionTable(*this), cfg(reporter, functionTable, symbolTable) {}

void Compiler::error(string err)
{
    ostringstream o;
    o << "Error: " << err << " whilst parsing line " << lookahead.line;
    throw runtime_error(o.str());
}

void Compiler::warning(string warn)
{
    reporter.warn(Reporter::AlertType::COMPILER, warn, lookahead.line);
}

string Compiler::quoteString(string &s)
{
    return "\"" + s + "\"";
}

void Compiler::compile(stringstream& out)
{
    tp = stream.cbegin();
    findGlobalsAndMakeStates();
    tp = stream.cbegin();
    lookahead = nextToken();
    while (lookahead.type != END) body();
    FunctionSymbol* mainFuncSym = functionTable.getFunction("main");
    cfg.setFirst(mainFuncSym->getFirstNode()->getName());
    cfg.setLast(mainFuncSym->getLastNode()->getName());
    Optimise::optimise(symbolTable, functionTable, cfg);
    SymbolicExecution::SymbolicExecutionManager(cfg, symbolTable, reporter).search();
    vector<Loop> loops = LengTarj(cfg).findLoops();

    for (Loop loop : loops)
    {
        cout << loop.getInfo() << "\n";
    }
    cout << cfg.getStructuredSource();
    //cout << cfg.destroyStructureAndGetFinalSource();
    //cout << cfg.getDotGraph();
}

Token Compiler::nextToken()
{
    return *(tp++);
}

shared_ptr<Identifier> Compiler::findVariable(string name)
{
    shared_ptr<Identifier> ret = symbolTable.findIdentifier(name);
    if (ret == nullptr) error("Undeclared variable '" + name + "'");
    return ret;
}

#define NUM_INITIAL 4
void Compiler::findGlobalsAndMakeStates()
{
    vector<unique_ptr<AbstractCommand>> initialState;
    string initialNames[NUM_INITIAL] = {"LHS", "RHS", "retD", "retS"};
    VariableType initialTypes[NUM_INITIAL] = {DOUBLE, DOUBLE, STRING, DOUBLE};

    for (int i = 0; i < NUM_INITIAL; ++i)
    {
        initialState.push_back(make_unique<DeclareVarCommand>(initialTypes[i], initialNames[i], -1));
        symbolTable.declare(initialTypes[i], initialNames[i], -1);
    }

    lookahead = nextToken();
    int depth = 0;
    while (lookahead.type != END)
    {
        if (lookahead.type == LBRACE)
        {
            ++depth;
            match(LBRACE);
        }

        else if (lookahead.type == RBRACE)
        {
            if (depth-- <= 0) error("Unexpected RBRACE");
            match(RBRACE);
        }

        else if (depth != 0) lookahead = nextToken();
        else
        {
            if (lookahead.type == FUNCTION)
            {
                match(FUNCTION);
                string id = lookahead.lexemeString;
                match(IDENT);
                match(LPAREN);

                vector<VariableType> paramTypes;
                if (lookahead.type != RPAREN)
                {
                    paramTypes.push_back((VariableType) lookahead.auxType);
                    match(DTYPE);
                    match(IDENT);
                    while (lookahead.type != RPAREN)
                    {
                        match(COMMA);
                        paramTypes.push_back((VariableType) lookahead.auxType);
                        match(DTYPE);
                        match(IDENT);
                    }
                }
                match(RPAREN);

                VariableType ret = VOID;
                if (lookahead.type != LBRACE)
                {
                    ret = (VariableType)lookahead.auxType;
                    match(DTYPE);
                }
                else warning("Function '" + id + "' has no dtype - assuming void");

                functionTable.addFunction(ret, paramTypes, id);
            }

            else //must be a global variable declaration
            {
                VariableType t = vtype();
                string id = ident();
                shared_ptr<Identifier> i = symbolTable.declare(t, id, lookahead.line);
                initialState.push_back(make_unique<DeclareVarCommand>(t, i->getUniqueID(), lookahead.line));
                if (lookahead.type == ASSIGN)
                {
                    match(ASSIGN);
                    i->setDefined();
                    if (t == STRING && lookahead.type == STRINGLIT)
                    {
                        initialState.push_back(make_unique<AssignVarCommand>
                                                       (i->getUniqueID(), lookahead.lexemeString, lookahead.line));
                        match(STRINGLIT);
                    }
                    else if (t == DOUBLE && lookahead.type == NUMBER)
                    {
                        initialState.push_back(make_unique<AssignVarCommand>
                                                       (i->getUniqueID(), lookahead.lexemeString, lookahead.line));
                        match(NUMBER);
                    }
                    else error("Can only assign literals in global scope");
                }
                match(SEMIC);
            }
        }
    }

    if (!functionTable.containsFunction("main")) error("Function 'main' must be defined");
    FunctionSymbol* mainSymbol = functionTable.getFunction("main");
    mainSymbol->addCommands(initialState);
}

void Compiler::match(Type t)
{
    if (lookahead.type != t)
    {
        ostringstream o;
        o << "Expected '" << TypeEnumNames[t] << "', found '" << TypeEnumNames[lookahead.type]
          << "' on line " << lookahead.line;
        throw runtime_error(o.str());
    }

    lookahead = nextToken();
}
