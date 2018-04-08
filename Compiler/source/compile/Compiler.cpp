//
// Created by abdul on 10/08/17.
//
#include <iostream>

#include "Compiler.h"
#include "../CFGOpt/Optimiser.h"
#include "../CFGOpt/LengTarj.h"
#include "../symbolic/VarWrappers.h"

using namespace std;

Compiler::Compiler(vector<Token>& st, Reporter& r):
        stream(st), reporter(r), functionTable(*this),
        cfg(r, functionTable, symbolTable) {}

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

Token Compiler::nextToken()
{
    return *(tp++);
}

void Compiler::compile(bool optimise, bool deadcode, bool verify, std::string graphOutput, bool gb, std::string outputfile)
{
    tp = stream.cbegin();
    findGlobalsAndMakeStates();
    tp = stream.cbegin();
    lookahead = nextToken();
    while (lookahead.type != END) body();

    FunctionSymbol* mainFuncSym = functionTable.getFunction("main");
    cfg.setFirst(mainFuncSym->getFirstNode()->getName());
    cfg.setLast(mainFuncSym->getLastNode()->getName());

    if (optimise) Optimise::optimise(symbolTable, functionTable, cfg);

    if (verify)
    {
        if (!graphOutput.empty() && gb)
        {
            ofstream fout(graphOutput);
            if (!fout.good()) throw runtime_error("Unable to open DOT graph output file '" + graphOutput + "'");
            fout << cfg.getDotGraph();
            fout.close();
        }

        SymbolicExecution::SymbolicExecutionManager symbolicExecutionManager
                = SymbolicExecution::SymbolicExecutionManager(cfg, symbolTable, reporter);
        unordered_map<string, SRPointer>& tags
                = symbolicExecutionManager.search(deadcode);

        if (!graphOutput.empty() && !gb)
        {
            ofstream fout(graphOutput);
            if (!fout.good()) throw runtime_error("Unable to open DOT graph output file '" + graphOutput + "'");
            fout << cfg.getDotGraph();
            fout.close();
        }

        vector<unique_ptr<Loop>> loops = LengTarj(cfg).findLoops();
        for (auto& loop : loops) loop->validate(tags);
    }

    if (!outputfile.empty())
    {
        fstream fout(outputfile);
        if (!fout.good()) throw runtime_error("Could not open filename '" + outputfile + "' for produced output.");
        fout << cfg.getStructuredSource() << "\n";
        fout.close();
    }
}

Identifier* Compiler::findVariable(VarWrapper* vg, VariableType* vtype)
{
    Identifier* ret = symbolTable.findIdentifier(vg->getBaseName());
    if (ret == nullptr) error("Undeclared variable '" + vg->getBaseName() + "'");
    if (vtype) *vtype = ret->getType() == ARRAY ? DOUBLE : ret->getType();
    return ret;
}

#define NUM_INITIAL 3
void Compiler::findGlobalsAndMakeStates()
{
    vector<unique_ptr<AbstractCommand>> initialState;
    string initialNames[NUM_INITIAL] = {"LHS", "RHS", "retD"};

    for (int i = 0; i < NUM_INITIAL; ++i)
    {
        initialState.push_back(make_unique<DeclareVarCommand>(initialNames[i], -1));
        symbolTable.declare(DOUBLE, initialNames[i], -1);
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
                unsigned int size;
                VariableType t = vtype(&size);
                string id = plainIdent();

                Identifier* i = symbolTable.declare(t, id, lookahead.line);
                if (t == DOUBLE)
                {
                    initialState.push_back(make_unique<DeclareVarCommand>(i->getUniqueID(), lookahead.line));
                }
                else if (t == ARRAY)
                {
                    initialState.push_back(make_unique<DeclareArrayCommand>(i->getUniqueID(), size, lookahead.line));
                }
                else throw runtime_error("Only support DOUBLE and ARRAY");

                if (lookahead.type == ASSIGN)
                {
                    match(ASSIGN);
                    i->setDefined();
                    auto iptr = make_unique<SDByName>(i->getUniqueID());
                    if (t == DOUBLE && lookahead.type == NUMBER)
                    {
                        initialState.push_back(make_unique<AssignVarCommand>
                                                       (move(iptr), Atom(stod(lookahead.lexemeString)), lookahead.line));
                        match(NUMBER);
                    }
                    else error("Can only assign double literals in global scope");
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
