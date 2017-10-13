//
// Created by abdul on 10/08/17.
//
#include <iostream>
#include <algorithm>

#include "../CFGOpt/Optimiser.h"
#include "Compiler.h"

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
    int debug = 0;
    for (const auto& node : cfg.getCurrentNodes()) if ((*node.second).getParentFunction()->getIdent() == "loopbody") debug++;
    bool debug2 = cfg.getNode("F2_loopbody_fin")->isLastNode() && cfg.getNode("F2_loopbody_fin")->isFirstNode();
    //SymbolicExecution::SymbolicExecutionManager symMan(cfg, symbolTable, reporter);
    //symMan.search();
    //Optimise::optimise(symbolTable, cfg);
    cout << cfg.getBinarySource();
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

void Compiler::findGlobalsAndMakeStates()
{
    vector<unique_ptr<AbstractCommand>> initialState;
    initialState.push_back(make_unique<DeclareVarCommand>(DOUBLE, "LHS", -1));
    initialState.push_back(make_unique<DeclareVarCommand>(DOUBLE, "RHS", -1));
    initialState.push_back(make_unique<DeclareVarCommand>(STRING, "retS", -1));
    initialState.push_back(make_unique<DeclareVarCommand>(DOUBLE, "retD", -1));

    lookahead = nextToken();
    bool globals = false;
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
                globals = true;
                VariableType t = vtype();
                string id = ident();
                shared_ptr<Identifier> i = symbolTable.declare(id, t, lookahead.line);
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
