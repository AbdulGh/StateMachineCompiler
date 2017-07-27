#include <sstream>
#include <iostream>
#include <stack>
#include <iostream>

#include "Compiler.h"
#include "../CFGOpt/Optimiser.h"

using namespace std;

Compiler::Compiler(vector<Token>& st, std::string auxFileName): stream(st), reporter(auxFileName) {}

void Compiler::error(string err)
{
    ostringstream o;
    o << "Error: " << err << " whilst parsing line " << lookahead.line;
    throw runtime_error(o.str());
}

void Compiler::warning(string warn)
{
    ostringstream o;
    o << warn << " (line " << lookahead.line << ")\n";
    reporter.warn(Reporter::AlertType::COMPILER, o.str());
}

string Compiler::quoteString(std::string &s)
{
    return "\"" + s + "\"";
}

void Compiler::compile(stringstream& out)
{
    tp = stream.cbegin();
    findGlobalsAndMakeFirstState();
    tp = stream.cbegin();
    lookahead = nextToken();
    while (lookahead.type != END) body();

    Optimise::optimise(symbolTable, cfg);

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

void Compiler::findGlobalsAndMakeFirstState()
{
    std::vector<shared_ptr<AbstractCommand>> initialState =
    {
            shared_ptr<AbstractCommand>(new DeclareVariableCommand(DOUBLE, "retD")),
            shared_ptr<AbstractCommand>(new DeclareVariableCommand(STRING, "retS")),
            shared_ptr<AbstractCommand>(new DeclareVariableCommand(DOUBLE, "LHS")),
            shared_ptr<AbstractCommand>(new DeclareVariableCommand(DOUBLE, "RHS"))
    };

    lookahead = nextToken();
    int depth = 0;
    while (lookahead.type != END)
    {
        if (lookahead.type == LBRACE)
        {
            depth++;
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

                FunctionPointer ptr(new FunctionCodeGen(ret, paramTypes, id, cfg));
                functionTable[id] = ptr;
            }

            else //must be a global variable declaration
            {
                VariableType t = vtype();
                string id = ident();
                shared_ptr<Identifier> i = symbolTable.declare(id, t, lookahead.line);
                initialState.push_back(shared_ptr<AbstractCommand>(new DeclareVariableCommand(t, i->getUniqueID())));
                if (lookahead.type == ASSIGN)
                {
                    match(ASSIGN);
                    i->setDefined();
                    if (t == STRING && lookahead.type == STRINGLIT)
                    {
                        initialState.push_back(shared_ptr<AbstractCommand>
                                                       (new AssignVarCommand(i->getUniqueID(), lookahead.lexemeString)));
                        match(STRINGLIT);
                    }
                    else if (t == DOUBLE && lookahead.type == NUMBER)
                    {
                        initialState.push_back(shared_ptr<AbstractCommand>
                                                       (new AssignVarCommand(i->getUniqueID(), lookahead.lexemeString)));
                        match(NUMBER);
                    }
                    else error("Can only assign literals in global scope");
                }
                match(SEMIC);
            }
        }
    }

    initialState.push_back(shared_ptr<AbstractCommand>(new JumpCommand("F_main_0")));

    cfg.addNode("start", initialState);
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

void Compiler::body()
{
    if (lookahead.type == DTYPE) //already checked
    {
        while (lookahead.type != Type::SEMIC) lookahead = nextToken();
        match(Type::SEMIC);
    }

    else if (lookahead.type == FUNCTION)
    {
        match(FUNCTION);
        string funName = ident();
        FunctionPointer fs = functionTable[funName];

        //get stack parameters into variables
        symbolTable.pushScope();
        match(LPAREN);
        stack<shared_ptr<AbstractCommand>> argumentStack;
        while (lookahead.type != RPAREN)
        {
            VariableType t = vtype();
            unsigned int line = lookahead.line;
            string s = ident();
            shared_ptr<Identifier> vid = symbolTable.declare(s, t, line);
            argumentStack.push(shared_ptr<AbstractCommand>(new PopCommand(vid->getUniqueID())));
            argumentStack.push(shared_ptr<AbstractCommand>(new DeclareVariableCommand(t, vid->getUniqueID())));
            if (lookahead.type == COMMA)
            {
                match(COMMA);
                if (lookahead.type == RPAREN) warning("Unexpected comma in parameters for function '" + funName + "' - ignoring");
            }
        }
        match(RPAREN);
        if (lookahead.type == DTYPE) match(DTYPE);

        while (!argumentStack.empty())
        {
            fs->addCommand(argumentStack.top());
            argumentStack.pop();
        }

        if (!statement(fs))
        {
            fs->genReturn();
            fs->genEndState();
        }
        symbolTable.popScope();
    }
    else match(END);
}

bool Compiler::statement(FunctionPointer fs)
{
    bool finishedState = false;
    if (lookahead.type == LBRACE)
    {
        match(LBRACE);
        symbolTable.pushScope();
        finishedState = statement(fs);
        while(lookahead.type != RBRACE) finishedState =  statement(fs);
        symbolTable.popScope();
        match(RBRACE);
    }
    else if (lookahead.type == PRINT)
    {
        match(PRINT);
        match(LPAREN);
        if (lookahead.type == STRINGLIT)
        {
            fs->genPrint(quoteString(lookahead.lexemeString));
            match(STRINGLIT);
        }
        else if (lookahead.type == NUMBER)
        {
            fs->genPrint(lookahead.lexemeString);
            match(NUMBER);
        }
        else
        {
            shared_ptr<Identifier> id = findVariable(ident());
            fs->genPrint(id->getUniqueID());
        }
        match(RPAREN);
        match(SEMIC);
    }
    else if (lookahead.type == INPUT)
    {
        match(INPUT);
        string id = ident();
        shared_ptr<Identifier> idPtr = findVariable(id);
        fs->genInput(idPtr->getUniqueID());
        idPtr->setDefined();
        match(SEMIC);
    }
    else if (lookahead.type == DTYPE)
    {
        VariableType t = vtype();
        string id = ident();
        shared_ptr<Identifier> idPtr =  symbolTable.declare(id, t, lookahead.line);
        fs->genVariableDecl(t, idPtr->getUniqueID());

        if (lookahead.type == ASSIGN)
        {
            match(ASSIGN);
            if (t == VariableType::STRING)
            {
                if (lookahead.type == CALL)
                {
                    genFunctionCall(t, fs);
                    fs->genAssignment(idPtr->getUniqueID(), "retS");
                }
                else
                {
                    fs->genAssignment(idPtr->getUniqueID(), quoteString(lookahead.lexemeString));
                    match(STRINGLIT);
                }
            }
            else expression(fs, idPtr->getUniqueID());
            idPtr->setDefined();
        }
        match(SEMIC);
    }
    else if (lookahead.type == IDENT)
    {
        shared_ptr<Identifier> idPtr = findVariable(ident());
        match(ASSIGN);
        if (idPtr->getType() == VariableType::STRING)
        {
            if (lookahead.type == STRINGLIT)
            {
                fs->genAssignment(idPtr->getUniqueID(), quoteString(lookahead.lexemeString));
                match(STRINGLIT);
            }
            else if (lookahead.type == CALL)
            {
                genFunctionCall(VariableType::STRING, fs);
                fs->genAssignment(idPtr->getUniqueID(), "retS");
            }
            else error("Malformed assignment to string");
        }
        else expression(fs, idPtr->getUniqueID());
        idPtr->setDefined();
        match(SEMIC);
    }
    else if (lookahead.type == CALL)
    {
        genFunctionCall(ANY, fs);
        match(SEMIC);
    }
    else if (lookahead.type == IF) genIf(fs);
    else if (lookahead.type == WHILE) genWhile(fs);
    else if (lookahead.type == RETURN)
    {
        finishedState = true;
        match(RETURN);
        if (lookahead.type != SEMIC)
        {
            if (lookahead.type == STRINGLIT)
            {
                if (fs->getReturnType() != STRING) error("Cannot return string in function of type " + fs->getReturnType());
                fs->genAssignment("retS", quoteString(lookahead.lexemeString));
                match(STRINGLIT);
            }
            else if (lookahead.type == NUMBER)
            {
                if (fs->getReturnType() != DOUBLE) error("Cannot return double in function of type " + fs->getReturnType());
                fs->genAssignment("retD", lookahead.lexemeString);
                match(NUMBER);
            }
            else
            {
                shared_ptr<Identifier> id = findVariable(ident());

                if (id->getType() == DOUBLE)
                {
                    if (fs->getReturnType() != DOUBLE) error("Cannot return double in function of type " + fs->getReturnType());
                    fs->genAssignment("retD", id->getUniqueID());
                }
                else
                {
                    if (fs->getReturnType() != STRING) error("Cannot return string in function of type " + fs->getReturnType());
                    fs->genAssignment("retS", id->getUniqueID());
                    match(STRINGLIT);
                }
            }
        }
        match(SEMIC);
        fs->genReturn();
        fs->genEndState();
    }
    return finishedState;
}

Relop Compiler::relop()
{
    Relop r = (Relop)lookahead.auxType;
    match(RELOP);
    return r;
}

void Compiler::expression(FunctionPointer fs, const std::string& to)
{
    ExpressionCodeGenerator gen(*this, to);
    gen.CompileExpression(fs);
}

VariableType Compiler::vtype()
{
    VariableType t = lookahead.auxType;
    match(DTYPE);
    return t;
}

string Compiler::ident()
{
    string s = lookahead.lexemeString;
    Compiler::match(IDENT);
    return s;
}