#include <sstream>
#include <iostream>
#include <stack>

#include "Compiler.h"
#include "../symbolic/VarWrappers.h"
#include "ExpressionCodeGenerator.h"

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

        std::string funName = plainIdent();

        FunctionSymbol* fs = functionTable.getFunction(funName);

        //get stack parameters into variables
        symbolTable.pushScope();
        match(LPAREN);
        stack<unique_ptr<AbstractCommand>> argumentStack;
        while (lookahead.type != RPAREN)
        {
            VariableType t = vtype();
            unsigned int line = lookahead.line;
            string s = plainIdent();
            if (lookahead.type != COMMA && lookahead.type != RPAREN) error("'" + s + "' is not a valid function parameter");
            Identifier* vid = symbolTable.declare(t, s, line);
            vid->setDefined();
            const string& vidName = vid->getUniqueID();
            argumentStack.push(make_unique<PopCommand>(make_unique<SVByName>(vidName), lookahead.line));
            argumentStack.push(make_unique<DeclareVarCommand>(vidName, lookahead.line));
            fs->addVar(new SVByName(vid->getUniqueID()));
            if (lookahead.type == COMMA)
            {
                match(COMMA);
                if (lookahead.type == RPAREN)
                {
                    warning("Unexpected comma in parameters for function '" + fs->getIdent() + "' - ignoring");
                }
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
    if (lookahead.type == LBRACE) //todo make switch
    {
        match(LBRACE);
        symbolTable.pushScope();
        fs->pushScope();
        finishedState = statement(fs);
        while(lookahead.type != RBRACE) finishedState = statement(fs);
        symbolTable.popScope();
        fs->popScope();
        match(RBRACE);
    }
    else if (lookahead.type == NONDET)
    {
        match(NONDET);
        Identifier* id;
        unique_ptr<VarWrapper> vw = wrappedIdent(&id);
        id->setDefined();
        if (id->getType() == VariableType::ARRAY) fs->genNondet(id->getLexeme(), lookahead.line);
        else fs->genNondet(move(vw), lookahead.line);
    }
    else if (lookahead.type == PRINT)
    {
        match(PRINT);
        match(LPAREN);
        while (lookahead.type != RPAREN)
        {
            if (lookahead.type == NUMBER)
            {
                fs->genPrintLiteral(lookahead.lexemeString, lookahead.line);
                match(NUMBER);
            }
            else
            {
                Atom a(wrappedIdent());
                fs->genPrintAtom(a, lookahead.line);
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
        Identifier* id;
        auto vw = wrappedIdent(&id);
        if (id->getType() != ARRAY) id->setDefined();
        fs->genInput(move(vw), lookahead.line);
        match(SEMIC);
    }
    else if (lookahead.type == DTYPE)
    {
        unsigned int size = 0;
        VariableType t = vtype(&size);
        auto id = plainIdent();
        if (symbolTable.isInScope(id)) error("Variable '" + id + "' is already in scope");
        else
        {
            Identifier* idPtr = symbolTable.declare(t, id, lookahead.line);
            if (t == ARRAY) fs->genArrayDecl(idPtr->getUniqueID(), size, lookahead.line);
            else fs->genVariableDecl(idPtr->getUniqueID(), lookahead.line);

            if (lookahead.type == ASSIGN)
            {
                if (t == ARRAY) error("Cannot assign into entire array");
                match(ASSIGN);
                unique_ptr<VarWrapper> vs = make_unique<SVByName>(idPtr->getUniqueID());
                expression(fs, move(vs));
                idPtr->setDefined();
            }
        }
        if (lookahead.type == COMMA)
        {
            match(COMMA);
            if (lookahead.type != DTYPE) error("Can only string together variable declatations with dtypes");
        }
        else match(SEMIC);
    }
    else if (lookahead.type == IDENT)
    {
        Identifier* idPtr;
        std::unique_ptr<VarWrapper> setter = wrappedIdent(&idPtr);
        match(ASSIGN);
        expression(fs, move(setter));
        idPtr->setDefined();
        match(SEMIC);
    }
    else if (lookahead.type == CALL)
    {
        genFunctionCall(fs, ANY);
        match(SEMIC);
    }
    else if (lookahead.type == IF) genIf(fs);
    else if (lookahead.type == WHILE) genWhile(fs);
    else if (lookahead.type == RETURN)
    {
        finishedState = true;
        match(RETURN);
        if (lookahead.type != SEMIC) ExpressionCodeGenerator(*this, make_unique<SVByName>("retD")).compileExpression(fs);
        else if (fs->getReturnType() != VOID) error("Void function '" + fs->getIdent() + "' returns some value");
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

void Compiler::expression(FunctionSymbol* fs, unique_ptr<VarWrapper> to)
{
    ExpressionCodeGenerator gen(*this, move(to));
    gen.compileExpression(fs);
}

VariableType Compiler::vtype(unsigned int* size)
{
    VariableType t = lookahead.auxType;
    match(DTYPE);
    if (lookahead.type == LSQPAREN)
    {
        match(LSQPAREN);
        if (lookahead.type != NUMBER) error("Expected number for array size");
        if (size != nullptr) *size = stoi(lookahead.lexemeString);
        match(NUMBER);
        match(RSQPAREN);
        t = ARRAY;
    }
    return t;
}

unique_ptr<VarWrapper> Compiler::wrappedIdent(Identifier** idp)
{
    string s = lookahead.lexemeString;
    Compiler::match(IDENT);
    Identifier* id = symbolTable.findIdentifier(s);
    if (!id) throw runtime_error("Could not find identifier '" + s + "'");
    if (idp) *idp = id;
    s = id->getUniqueID();
    if (lookahead.type == LSQPAREN)
    {
        match(LSQPAREN);

        if (lookahead.type == NUMBER)
        {
            int index = stoi(lookahead.lexemeString);
            match(NUMBER);
            match(RSQPAREN);
            return make_unique<SDByArrayIndex>(s, index);
        }
        else
        {
            unique_ptr<VarWrapper> indexVar = wrappedIdent();
            match(RSQPAREN);
            return make_unique<SDByIndexVar>(s, move(indexVar));
        }
    }
    else return make_unique<SVByName>(s);
}

std::string Compiler::plainIdent()
{
    string s = lookahead.lexemeString;
    Compiler::match(IDENT);
    return s;
}