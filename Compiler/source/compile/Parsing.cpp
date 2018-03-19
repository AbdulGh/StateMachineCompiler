#include <sstream>
#include <iostream>
#include <stack>

#include "Compiler.h"
#include "VarWrappers.h"
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

        unique_ptr<VarWrapper> funName = identGetter();

        FunctionSymbol* fs = functionTable.getFunction(funName->getFullName());

        //get stack parameters into variables
        symbolTable.pushScope();
        match(LPAREN);
        stack<unique_ptr<AbstractCommand>> argumentStack;
        while (lookahead.type != RPAREN)
        {
            VariableType t = vtype();
            unsigned int line = lookahead.line;
            AccessType atcheck;
            unique_ptr<VarWrapper> s = identSetter(&atcheck);
            if (atcheck != AccessType::DIRECT) error("'" + s->getFullName() + "' is not a valid function parameter");
            Identifier* vid = symbolTable.declare(t, s->getFullName(), line);
            vid->setDefined();
            const string& vidName = vid->getUniqueID();
            argumentStack.push(make_unique<PopCommand>(move(s), lookahead.line));
            argumentStack.push(make_unique<DeclareVarCommand>(t, vidName, lookahead.line));
            fs->addVar(s->clone().release());
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
    if (lookahead.type == LBRACE)
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
    else if (lookahead.type == PRINT)
    {
        match(PRINT);
        match(LPAREN);
        while (lookahead.type != RPAREN)
        {
            if (lookahead.type == STRINGLIT)
            {
                Atom a(quoteString(lookahead.lexemeString));
                fs->genPrint(a, lookahead.line);
                match(STRINGLIT);
            }
            else if (lookahead.type == NUMBER)
            {
                Atom a(lookahead.lexemeString);
                fs->genPrint(a, lookahead.line);
                match(NUMBER);
            }
            else
            {
                Atom a(identGetter());
                fs->genPrint(a,  lookahead.line);
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
        auto id = identSetter();
        Identifier* idPtr = findVariable(id.get());
        if (idPtr->getType() != ARRAY) idPtr->setDefined();
        fs->genInput(move(id), lookahead.line);
        match(SEMIC);
    }
    else if (lookahead.type == DTYPE)
    {
        unsigned int size = 0;
        VariableType t = vtype(&size);
        auto id = identGetter();
        if (symbolTable.isInScope(id->getBaseName())) //set to '0' or '""' depending on type
        {
            error("Variable '" + id->getBaseName() + "' is already in scope");
            /*string uid; VariableType vtype;
            Identifier* idPtr = findVariable(id, uid, &vtype);
            if (t != idPtr->getType()) error("'" + id + "' redeclared in same scope with different type");
            else if (lookahead.type == ASSIGN)
            {
                match(ASSIGN);
                if (t == STRING)
                {
                    if (lookahead.type == CALL) genFunctionCall(fs, vtype, uid);
                    else if (lookahead.type == STRINGLIT)
                    {
                        fs->genAssignment(uid, quoteString(lookahead.lexemeString), lookahead.line);
                        match(STRINGLIT);
                    }
                    else
                        error("'" + id + "' declared in this scope as a string, assigned to "
                              + TypeEnumNames[lookahead.type]);
                }
                else expression(fs, uid);
            }
            else
            {
                if (idPtr->getType() == DOUBLE || idPtr->getType() == ARRAY) fs->genAssignment(uid, "0", lookahead.line);
                else if (idPtr->getType() == STRING) fs->genAssignment(uid, "\"\"", lookahead.line);
                else throw "bad dtype";
            }*/
        }
        else
        {
            Identifier* idPtr = symbolTable.declare(t, id->getBaseName(), lookahead.line);
            if (t == ARRAY) fs->genArrayDecl(idPtr->getUniqueID(), size, lookahead.line);
            else fs->genVariableDecl(t, idPtr->getUniqueID(), lookahead.line);

            if (lookahead.type == ASSIGN)
            {
                if (t == ARRAY) error("Cannot assign into entire array");
                match(ASSIGN);
                unique_ptr<VarWrapper> vs = make_unique<SVByName>(idPtr->getUniqueID());

                if (t == VariableType::STRING)
                {
                    if (lookahead.type == CALL)
                    {
                        genFunctionCall(fs, STRING, move(vs));
                    }
                    else
                    {
                        fs->genAssignment(move(vs), quoteString(lookahead.lexemeString), lookahead.line);
                        match(STRINGLIT);
                    }
                }
                else expression(fs, move(vs));
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
        std::unique_ptr<VarWrapper> setter = identSetter();
        Identifier* idPtr = findVariable(setter.get());
        match(ASSIGN);
        if (idPtr->getType() == VariableType::STRING)
        {
            if (lookahead.type == STRINGLIT)
            {
                fs->genAssignment(move(setter), quoteString(lookahead.lexemeString), lookahead.line);
                match(STRINGLIT);
            }
            else if (lookahead.type == CALL) genFunctionCall(fs, idPtr->getType(), move(setter));
            else error("Malformed assignment to string");
        }
        else expression(fs, move(setter));
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
        if (lookahead.type != SEMIC)
        {
            if (lookahead.type == STRINGLIT)
            {
                if (fs->getReturnType() != STRING) error("Cannot return string in function of type " + fs->getReturnType());
                fs->genAssignment(make_unique<SVByName>("retS"), quoteString(lookahead.lexemeString), lookahead.line);
                match(STRINGLIT);
            }
            else ExpressionCodeGenerator(*this, make_unique<SVByName>("retD")).compileExpression(fs);
        }
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

unique_ptr<VarWrapper> Compiler::identGetter(AccessType* at)
{
    string s = lookahead.lexemeString;
    Compiler::match(IDENT);
    if (lookahead.type == LSQPAREN)
    {
        if (at != nullptr) *at = AccessType::BYARRAY;
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
            unique_ptr<VarWrapper> indexVar = identGetter();
            match(RSQPAREN);
            return make_unique<SDByIndexVar>(s, move(indexVar));
        }
    }
    else
    {
        if (at != nullptr) *at = AccessType::DIRECT;
        return make_unique<SVByName>(s);
    }
}

unique_ptr<VarWrapper> Compiler::identSetter(AccessType* at)
{
    string s = lookahead.lexemeString;
    Compiler::match(IDENT);
    if (lookahead.type == LSQPAREN)
    {
        if (at != nullptr) *at = AccessType::BYARRAY;
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
            unique_ptr<VarWrapper> indexVar = identGetter();
            match(RSQPAREN);
            return make_unique<SDByIndexVar>(s, move(indexVar));
        }
    }
    else
    {
        if (at != nullptr) *at = AccessType::DIRECT;
        return make_unique<SVByName>(s);
    }
}

std::string Compiler::identPlain()
{
    string s = lookahead.lexemeString;
    Compiler::match(IDENT);
    return s;
}