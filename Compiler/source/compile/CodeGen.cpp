#include "Compiler.h"

using namespace std;
VariableType Compiler::genFunctionCall(VariableType expectedType, shared_ptr<FunctionCodeGen> fromFS)
{
    match(Type::CALL);
    string fid = ident();
    FunctionCodeGen& toFS = *(findFunction(fid));
    if (expectedType != ANY && !toFS.isOfType(expectedType)) error("Type error");
        //error ("Function '" + fid + "' returns type " + toFS.getReturnType() + ", expected " + expectedType);
    match(Type::LPAREN);

    string nextState = fromFS->newStateName();
    fromFS->genPush("state " + nextState);

    vector<VariableType> paramTypes;
    while (lookahead.type != Type::RPAREN)
    {
        if (lookahead.type == Type::NUMBER)
        {
            string toPush = lookahead.lexemeString;
            match(Type::NUMBER);
            paramTypes.push_back(VariableType::DOUBLE);
            fromFS->genPush(toPush);
        }
        else if (lookahead.type == Type::STRINGLIT)
        {
            string toPush = lookahead.lexemeString;
            match(Type::STRINGLIT);
            paramTypes.push_back(VariableType::STRING);
            fromFS->genPush(quoteString(toPush));
        }
        else
        {
            string iid = ident();
            shared_ptr<Identifier> idp = findVariable(iid);
            paramTypes.push_back(idp->getType());
            fromFS->genPush(idp->getUniqueID());
        }
        if (lookahead.type == Type::COMMA)
        {
            match(Type::COMMA);
            if (lookahead.type == Type::RPAREN) warning("Unexpected comma in arguments for function '" + fid + "' - ignoring");
        }
    }
    match(Type::RPAREN);

    if (!toFS.checkTypes(paramTypes)) error("Type mismatch for function '" + fid + "'");

    fromFS->genJump("jump F_" + fid + "_0");
    fromFS->genEndState();
    fromFS->genNewState(nextState);
    return toFS.getReturnType();
}

void Compiler::genIf(FunctionPointer fs)
{
    string success = fs->newStateName();
    string fail = fs->newStateName();

    match(IF);
    match(LPAREN);
    ors(fs, success, fail);
    match(RPAREN);
    match(LBRACE);
    symbolTable.pushScope();

    fs->genNewState(success);
    statement(fs);
    while (lookahead.type != RBRACE) statement(fs);

    match(RBRACE);
    symbolTable.popScope();

    if (lookahead.type == ELSE)
    {
        match(ELSE);
        match(LBRACE);
        symbolTable.pushScope();
        string skip = fs->newStateName();
        fs->genJump(skip);
        fs->genEndState();
        fs->genNewState(fail);
        statement(fs);
        fs->genJump(skip);
        fs->genEndState();
        fs->genNewState(skip);
        match(RBRACE);
        symbolTable.popScope();
    }
    else
    {
        fs->genJump(fail);
        fs->genEndState();
        fs->genNewState(fail);
    }
}

void Compiler::genWhile(FunctionPointer fs)
{
    string loopcheck = fs->newStateName();
    string body = fs->newStateName();
    string end = fs->newStateName();

    fs->genJump(loopcheck);
    fs->genEndState();
    fs->genNewState(loopcheck);

    match(WHILE);
    match(LPAREN);
    ors(fs, body, end);
    match(RPAREN);
    match(LBRACE);
    symbolTable.pushScope();

    fs->genNewState(body);
    statement(fs);
    while (lookahead.type != RBRACE) statement(fs);

    match(RBRACE);
    symbolTable.popScope();
    fs->genJump(loopcheck);
    fs->genEndState();
    fs->genNewState(end);
}

void Compiler::ors(FunctionPointer fs, string success, string fail)
{
    string IM = fs->newStateName();
    ands(fs, success, IM);
    while (lookahead.type == COMPOR)
    {
        match(COMPOR);
        fs->genNewState(IM);
        IM = fs->newStateName();
        ands(fs, success, IM);
    }

    fs->genNewState(IM);
    fs->genJump(fail);
    fs->genEndState();
}

void Compiler::ands(FunctionPointer fs, string success, string fail)
{
    string IM = fs->newStateName();
    condition(fs, IM, fail);
    while (lookahead.type == COMPAND)
    {
        match(COMPAND);
        fs->genNewState(IM);
        IM = fs->newStateName();
        condition(fs, IM, fail);
    }

    fs->genNewState(IM);
    fs->genJump(success);
    fs->genEndState();
}

void Compiler::condition(FunctionPointer fs, string success, string fail)
{
    expression(fs, "LHS");
    Relop r = relop();
    expression(fs, "RHS");

    fs->genConditionalJump(success, "LHS", r, "RHS");
    fs->genJump(fail);
    fs->genEndState();
}