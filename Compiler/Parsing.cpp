#include <sstream>
#include <iostream>
#include <stack>

#include "Compiler.h"

using namespace std;

Compiler::Compiler(vector<Token>& st): stream(st) {}

void Compiler::error(string err)
{
    ostringstream o;
    o << "Error: " << err << " whilst parsing line " << lookahead.line;
    throw runtime_error(o.str());
}

void Compiler::compile(stringstream& out)
{
    tp = stream.cbegin();
    findFunctionsAndMakeFirstStates(out);
    tp = stream.cbegin();
    lookahead = nextToken();
    while (lookahead.type != END) body();

    for (unordered_map<string, FunctionPointer>::const_iterator it = functionTable.cbegin();
         it != functionTable.cend(); ++it)
    {
        out << it->second->getSource();
    }
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

void Compiler::findFunctionsAndMakeFirstStates(std::stringstream& out)
{
    out << "start\ndouble retD;\nstring retS;" << endl;
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
                else
                {
                    //todo warning - assuming void
                }

                FunctionPointer ptr(new FunctionSymbol(ret, paramTypes, id));
                functionTable[id] = ptr;
            }

            else //must be a global variable declaration
            {
                VariableType t = vtype();
                string id = ident();
                shared_ptr<Identifier> i = symbolTable.declare(id, t, lookahead.line);
                out << VariableTypeEnumNames[t] << " G_" << id << ";\n";
                if (lookahead.type == ASSIGN)
                {
                    match(ASSIGN);
                    i->setDefined();
                    if (t == STRING && lookahead.type == STRINGLIT)
                    {
                        out << "G_" << id << " = \"" << lookahead.lexemeString << "\";\n";
                        match(STRINGLIT);
                    }
                    else if (t == DOUBLE && lookahead.type == NUMBER)
                    {
                        out << "G_" << id << " = " << lookahead.lexemeString << ";\n";
                        match(NUMBER);
                    }
                    else error("Can only assign literals in global scope");
                }
                match(SEMIC);
            }
        }
    }
    out << "jump F_main_0;\nend\n\n";
    out << "funFinish\njump pop;\nend";
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
        fs->emit("\n\n" + fs->genNewStateName() + "\n");

        //get stack parameters into variables
        symbolTable.pushScope();
        match(LPAREN);
        stack<string> argumentStack;
        while (lookahead.type != RPAREN)
        {
            VariableType t = vtype();
            unsigned int line = lookahead.line;
            string s = ident();
            shared_ptr<Identifier> vid = symbolTable.declare(s, t, line);
            argumentStack.push("pop " + vid->getUniqueID() + ";\n");
            argumentStack.push(VariableTypeEnumNames[t] + " " + vid->getUniqueID() + ";\n");
            if (lookahead.type == COMMA)
            {
                match(COMMA);
                if (lookahead.type == RPAREN)
                {
                    //todo warning
                }
            }
        }
        match(RPAREN);
        if (lookahead.type == DTYPE) match(DTYPE);

        while (!argumentStack.empty())
        {
            fs->emit(argumentStack.top());
            argumentStack.pop();
        }

        if (!statement(fs)) fs->emit("jump funFinish;\nend");
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
            fs->emit("print \"" + lookahead.lexemeString + "\";\n");
            match(STRINGLIT);
        }
        else if (lookahead.type == NUMBER)
        {
            fs->emit("print " + lookahead.lexemeString + ";\n");
            match(NUMBER);
        }
        else
        {
            shared_ptr<Identifier> id = findVariable(ident());
            fs->emit("print " + id->getUniqueID() + ";\n");
        }
        match(RPAREN);
        match(SEMIC);
    }
    else if (lookahead.type == DTYPE)
    {
        VariableType t = vtype();
        string id = ident();
        shared_ptr<Identifier> idPtr =  symbolTable.declare(id, t, lookahead.line);

        if (lookahead.type == ASSIGN)
        {
            match(ASSIGN);
            if (t == VariableType::STRING)
            {
                if (lookahead.type == CALL)
                {
                    genFunctionCall(t, fs);
                    fs->emit(idPtr->getUniqueID() + "= retS;\n");
                }
                else
                {
                    fs->emit(idPtr->getUniqueID() + " = \"" + lookahead.lexemeString + "\";\n");
                    match(STRINGLIT);
                }
            }
            else expression(fs, idPtr->getUniqueID());
        }
        match(SEMIC);
    }
    else if (lookahead.type == IDENT)
    {
        shared_ptr<Identifier> id = findVariable(ident());
        match(ASSIGN);
        if (id->getType() == VariableType::STRING)
        {
            if (lookahead.type == STRINGLIT)
            {
                fs->emit(id->getUniqueID() + " = \"" + lookahead.lexemeString + "\";\n");
                match(STRINGLIT);
            }
            else if (lookahead.type == CALL)
            {
                genFunctionCall(VariableType::STRING, fs);
                fs->emit(id->getUniqueID() + "= retS;\n");
            }
            else error("Malformed assignment to string");
        }
        else expression(fs, id->getUniqueID());
        match(SEMIC);
    }
    else if (lookahead.type == CALL)
    {
        genFunctionCall(ANY, fs);
        match(SEMIC);
    }
    else if (lookahead.type == IF)
    {
        match(IF);
        match(LPAREN);
        ands(fs);
        match(RPAREN);
        statement(fs);
    }
    else if (lookahead.type == WHILE)
    {
        match(WHILE);
        match(LPAREN);
        ands(fs);
        match(RPAREN);
        statement(fs);
    }
    else if (lookahead.type == RETURN)
    {
        finishedState = true;
        match(RETURN);
        if (lookahead.type != SEMIC)
        {
            if (lookahead.type == STRINGLIT)
            {
                if (fs->getReturnType() != STRING) error("Cannot return string in function of type " + fs->getReturnType());
                fs->emit("retS = \"" + lookahead.lexemeString + "\";\n");
                match(STRINGLIT);
            }
            else if (lookahead.type == NUMBER)
            {
                if (fs->getReturnType() != DOUBLE) error("Cannot return double in function of type " + fs->getReturnType());
                fs->emit("retD = " + lookahead.lexemeString + ";\n");
                match(NUMBER);
            }
            else
            {
                shared_ptr<Identifier> id = findVariable(ident());

                if (id->getType() == DOUBLE)
                {
                    if (fs->getReturnType() != DOUBLE) error("Cannot return double in function of type " + fs->getReturnType());
                    fs->emit("retD = " + id->getUniqueID() + ";\n");
                }
                else
                {
                    if (fs->getReturnType() != STRING) error("Cannot return string in function of type " + fs->getReturnType());
                    fs->emit("retS = \"" + id->getUniqueID() + "\";\n");
                    match(STRINGLIT);
                }
            }
        }
        match(SEMIC);
        fs->emit("jump funFinish;\nend");
    }
    return finishedState;
}

void Compiler::ands(FunctionPointer fs)
{
    ors(fs);
    while (lookahead.type == COMPAND)
    {
        match(COMPAND);
        ors(fs);
    }
}

void Compiler::ors(FunctionPointer fs)
{
    condition(fs);
    while (lookahead.type == COMPOR)
    {
        match(COMPOR);
        condition(fs);
    }
}

void Compiler::condition(FunctionPointer fs)
{
    //todo conditions
    string todo = "todo";
    expression(fs, todo);
    relop();
    expression(fs, todo);
}

void Compiler::relop()
{
    match(RELOP);
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