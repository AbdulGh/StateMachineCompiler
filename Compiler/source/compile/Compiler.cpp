//
// Created by abdul on 10/08/17.
//
#include <iostream>
#include <algorithm>

#include "../CFGOpt/Optimiser.h"
#include "Compiler.h"

using namespace std;

Compiler::Compiler(vector<Token>& st, string auxFileName): stream(st), reporter(move(auxFileName)), cfg(reporter) {}

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

    cfg.setLast(functionTable["main"]->getLastNode()->getName());

    auto debugFunc = [&, this]() -> void
    {
        unordered_map<string, shared_ptr<CFGNode>>& list = this->cfg.getCurrentNodes();
        for (const auto& liasd : list)
        {
            for (const auto& asdf : liasd.second->getReturnSuccessors())
            {
                cout << liasd.first << " has as a succ " << asdf->getName() << "\n";
                if (asdf->getPredecessors().find(liasd.first) == asdf->getPredecessors().end())
                {
                    cout << "but that succ does not have it as a parent!\n";
                }
            }
            for (const auto& liasdParent : liasd.second->getPredecessors())
            {
                if (!((liasdParent.second->getCompSuccess() != nullptr
                       && liasdParent.second->getCompSuccess()->getName() == liasd.first)
                      || (liasdParent.second->getCompFail() != nullptr
                          && liasdParent.second->getCompFail()->getName() == liasd.first)
                    || ((find_if(liasdParent.second->getReturnSuccessors().begin(),
                                liasdParent.second->getReturnSuccessors().end(),
                                [&, liasd](shared_ptr<CFGNode> p) -> bool
                                {
                                    return p->getName() == liasd.second->getName();
                                })) != liasdParent.second->getReturnSuccessors().end()))) //haha
                {
                    cout << liasdParent.first << " does not recognise " << liasd.first << " as a child!\n";
                }
            }
        }
    };

    Optimise::optimise(symbolTable, cfg);
    debugFunc();
    //SymbolicExecution::SymbolicExecutionManager symMan(cfg, symbolTable, reporter);
    //symMan.search();
    //Optimise::optimise(symbolTable, cfg);
    out << cfg.getSource();
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

FunctionPointer Compiler::findFunction(string fid)
{
    unordered_map<string, FunctionPointer>::const_iterator it = functionTable.find(fid);
    if (it == functionTable.cend()) error("Undefined function '" + fid + "'");
    return it->second;
}

void Compiler::findGlobalsAndMakeStates()
{
    vector<shared_ptr<AbstractCommand>> initialState =
    {
        make_shared<DeclareVarCommand>(DOUBLE, "LHS", -1),
        make_shared<DeclareVarCommand>(DOUBLE, "RHS", -1),
        make_shared<DeclareVarCommand>(STRING, "retS", -1),
        make_shared<DeclareVarCommand>(DOUBLE, "retD", -1)
    };

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

                FunctionPointer ptr = make_shared<FunctionCodeGen>(ret, paramTypes, id, cfg);
                functionTable[id] = ptr;
            }

            else //must be a global variable declaration
            {
                VariableType t = vtype();
                string id = ident();
                shared_ptr<Identifier> i = symbolTable.declare(id, t, lookahead.line);
                initialState.push_back(make_shared<DeclareVarCommand>(t, i->getUniqueID(), lookahead.line));
                if (lookahead.type == ASSIGN)
                {
                    match(ASSIGN);
                    i->setDefined();
                    if (t == STRING && lookahead.type == STRINGLIT)
                    {
                        initialState.push_back(make_shared<AssignVarCommand>
                                                       (i->getUniqueID(), lookahead.lexemeString, lookahead.line));
                        match(STRINGLIT);
                    }
                    else if (t == DOUBLE && lookahead.type == NUMBER)
                    {
                        initialState.push_back(make_shared<AssignVarCommand>
                                                       (i->getUniqueID(), lookahead.lexemeString, lookahead.line));
                        match(NUMBER);
                    }
                    else error("Can only assign literals in global scope");
                }
                match(SEMIC);
            }
        }
    }

    initialState.push_back(make_shared<JumpCommand>("F_main_0", -1));
    cfg.createNode("start", nullptr, false, false)->setInstructions(initialState);
    cfg.setFirst("start");
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
