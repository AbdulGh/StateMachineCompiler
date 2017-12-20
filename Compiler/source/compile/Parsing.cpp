#include <sstream>
#include <iostream>
#include <stack>

#include "Compiler.h"


using namespace std;

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
        FunctionSymbol* fs = functionTable.getFunction(funName);

        //get stack parameters into variables
        symbolTable.pushScope();
        match(LPAREN);
        stack<unique_ptr<AbstractCommand>> argumentStack;
        while (lookahead.type != RPAREN)
        {
            VariableType t = vtype();
            unsigned int line = lookahead.line;
            string s = ident();
            shared_ptr<Identifier> vid = symbolTable.declare(t, s, line);
            vid->setDefined();
            const string& vidName = vid->getUniqueID();
            argumentStack.push(make_unique<PopCommand>(vidName, lookahead.line));
            argumentStack.push(make_unique<DeclareVarCommand>(t, vidName, lookahead.line));
            fs->addVar(vidName);
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
            fs->addCommand(move(argumentStack.top()));
            argumentStack.pop();
        }

        if (!statement(fs))
        {
            fs->genReturn(lookahead.line);
            fs->genEndState();
        }
        symbolTable.popScope();
    }
    else match(END);
}

bool Compiler::statement(FunctionSymbol* fs)
{
    bool finishedState = false;
    if (lookahead.type == LBRACE)
    {
        match(LBRACE);
        symbolTable.pushScope();
        finishedState = statement(fs);
        while(lookahead.type != RBRACE) finishedState = statement(fs);
        symbolTable.popScope();
        match(RBRACE);
    }
    else if (lookahead.type == PRINT)
    {
        match(PRINT);
        match(LPAREN);
        while (lookahead.type != RPAREN)
        {
            if (lookahead.type == STRINGLIT)
            {
                fs->genPrint(quoteString(lookahead.lexemeString), lookahead.line);
                match(STRINGLIT);
            }
            else if (lookahead.type == NUMBER)
            {
                fs->genPrint(lookahead.lexemeString, lookahead.line);
                match(NUMBER);
            }
            else
            {
                shared_ptr<Identifier> id = findVariable(ident());
                fs->genPrint(id->getUniqueID(), lookahead.line);
            }

            if (lookahead.type == COMMA)
            {
                match(COMMA);
                if (lookahead.type == RPAREN) warning("Extra comma found in print statement - ignoring");
            }
        }
        match(RPAREN);
        match(SEMIC);
    }
    else if (lookahead.type == INPUT)
    {
        match(INPUT);
        string id = ident();
        shared_ptr<Identifier> idPtr = findVariable(id);
        fs->genInput(idPtr->getUniqueID(), lookahead.line);
        idPtr->setDefined();
        match(SEMIC);
    }
    else if (lookahead.type == DTYPE)
    {
        VariableType t = vtype();
        string id = ident();
        if (symbolTable.isInScope(id)) //set to '0' or '""' depending on type
        {
            shared_ptr<Identifier> idPtr = findVariable(id);
            if (t != idPtr->getType()) error("'" + id + "' redeclared in same scope with different type");
            else if (lookahead.type == ASSIGN)
            {
                match(ASSIGN);
                if (t == STRING)
                {
                    if (lookahead.type == CALL) genFunctionCall(fs, idPtr);
                    else if (lookahead.type == STRINGLIT)
                    {
                        fs->genAssignment(idPtr->getUniqueID(), quoteString(lookahead.lexemeString), lookahead.line);
                        match(STRINGLIT);
                    }
                    else
                        error("'" + id + "' declared in this scope as a string, assigned to "
                              + TypeEnumNames[lookahead.type]);
                }
                else expression(fs, idPtr->getUniqueID());
            }
            else
            {
                shared_ptr<Identifier> ident = findVariable(id);
                if (ident->getType() == DOUBLE) fs->genAssignment(ident->getUniqueID(), "0", lookahead.line);
                else fs->genAssignment(ident->getUniqueID(), "\"\"", lookahead.line);
            }
        }
        else
        {
            shared_ptr<Identifier> idPtr = symbolTable.declare(t, id, lookahead.line);
            fs->genVariableDecl(t, idPtr->getUniqueID(), lookahead.line);

            if (lookahead.type == ASSIGN)
            {
                match(ASSIGN);
                if (t == VariableType::STRING)
                {
                    if (lookahead.type == CALL) genFunctionCall(fs, idPtr);
                    else
                    {
                        fs->genAssignment(idPtr->getUniqueID(), quoteString(lookahead.lexemeString), lookahead.line);
                        match(STRINGLIT);
                    }
                }
                else expression(fs, idPtr->getUniqueID());
                idPtr->setDefined();
            }
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
                fs->genAssignment(idPtr->getUniqueID(), quoteString(lookahead.lexemeString), lookahead.line);
                match(STRINGLIT);
            }
            else if (lookahead.type == CALL) genFunctionCall(fs, idPtr);
            else error("Malformed assignment to string");
        }
        else expression(fs, idPtr->getUniqueID());
        idPtr->setDefined();
        match(SEMIC);
    }
    else if (lookahead.type == CALL)
    {
        genFunctionCall(fs, nullptr);
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
                fs->genAssignment("retS", quoteString(lookahead.lexemeString), lookahead.line);
                match(STRINGLIT);
            }
            else if (lookahead.type == NUMBER)
            {
                if (fs->getReturnType() != DOUBLE) error("Cannot return double in function of type " + fs->getReturnType());
                fs->genAssignment("retD", lookahead.lexemeString, lookahead.line);
                match(NUMBER);
            }
            else
            {
                shared_ptr<Identifier> id = findVariable(ident());

                if (id->getType() == DOUBLE)
                {
                    if (fs->getReturnType() != DOUBLE) error("Cannot return double in function of type " + fs->getReturnType());
                    fs->genAssignment("retD", id->getUniqueID(), lookahead.line);
                }
                else
                {
                    if (fs->getReturnType() != STRING) error("Cannot return string in function of type " + fs->getReturnType());
                    fs->genAssignment("retS", id->getUniqueID(), lookahead.line);
                    match(STRINGLIT);
                }
            }
        }
        match(SEMIC);
        fs->genReturn(lookahead.line);
        fs->genEndState();
    }
    return finishedState;
}

Relations::Relop Compiler::relop()
{
    Relations::Relop r = (Relations::Relop)lookahead.auxType;
    match(RELOP);
    return r;
}

void Compiler::expression(FunctionSymbol* fs, const std::string& to)
{
    ExpressionCodeGenerator gen(*this, to);
    gen.compileExpression(fs);
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