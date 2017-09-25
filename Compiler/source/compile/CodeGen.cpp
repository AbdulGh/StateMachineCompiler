#include "Compiler.h"

using namespace std;
VariableType Compiler::genFunctionCall(shared_ptr<FunctionCodeGen> fromFS, shared_ptr<Identifier> toVar)
{
    match(Type::CALL);
    string fid = ident();
    FunctionCodeGen& toFS = *(findFunction(fid));
    VariableType expectedType;
    if (toVar != nullptr)
    {
        expectedType = toVar->getType();
        if (expectedType != ANY && !toFS.isOfType(expectedType))
        {
            error ("Function '" + fid + "' returns type " + TypeEnumNames[toFS.getReturnType()]
                   + ", expected " + TypeEnumNames[expectedType]);
        }
    }
    match(Type::LPAREN);

    //push all vars
    const vector<string>& fromVars = fromFS->getVars();
    for (const string& s : fromVars) fromFS->genPush(PushCommand::PUSHSTR, s, lookahead.line);

    string nextState = fromFS->newStateName();
    fromFS->genPush(PushCommand::PUSHSTATE, nextState, lookahead.line);

    vector<VariableType> paramTypes;
    while (lookahead.type != Type::RPAREN)
    {
        if (lookahead.type == Type::NUMBER)
        {
            string toPush = lookahead.lexemeString;
            match(Type::NUMBER);
            paramTypes.push_back(VariableType::DOUBLE);
            fromFS->genPush(PushCommand::PUSHSTR, toPush, lookahead.line);
        }
        else if (lookahead.type == Type::STRINGLIT)
        {
            string toPush = lookahead.lexemeString;
            match(Type::STRINGLIT);
            paramTypes.push_back(VariableType::STRING);
            fromFS->genPush(PushCommand::PUSHSTR, quoteString(toPush), lookahead.line);
        }
        else
        {
            string iid = ident();
            shared_ptr<Identifier> idp = findVariable(iid);
            paramTypes.push_back(idp->getType());
            fromFS->genPush(PushCommand::PUSHSTR, idp->getUniqueID(), lookahead.line);
        }
        if (lookahead.type == Type::COMMA)
        {
            match(Type::COMMA);
            if (lookahead.type == Type::RPAREN) warning("Unexpected comma in arguments for function '" + fid + "' - ignoring");
        }
    }
    match(Type::RPAREN);

    if (!toFS.checkTypes(paramTypes)) error("Type mismatch in parameters for function '" + fid + "'");

    fromFS->genJump(toFS.getFirstNode()->getName(), lookahead.line);
    fromFS->genEndState();
    fromFS->genNewState(nextState);
    shared_ptr<CFGNode> created = fromFS->getCurrentNode();
    created->addParent(toFS.getLastNode());

    //pop all vars back
    for (auto rit = fromVars.rbegin(); rit != fromVars.rend(); ++rit)
    {
        fromFS->genPop(*rit, lookahead.line);
    }

    if (toVar != nullptr)
    {
        switch (toFS.getReturnType())
        {
            case DOUBLE:
                fromFS->genAssignment(toVar->getUniqueID(), "retD", lookahead.line);
                break;
            case STRING:
                fromFS->genAssignment(toVar->getUniqueID(), "retS", lookahead.line);
                break;
            default:
                throw runtime_error("Unaccounted for variable type");
        }
    }
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

    fs->genNewState(success);

    bool finishedState;
    if (lookahead.type != LBRACE) finishedState = statement(fs);
    else
    {
        match(LBRACE);
        symbolTable.pushScope();
        finishedState = statement(fs);
        while (lookahead.type != RBRACE) finishedState = statement(fs);
        match(RBRACE);
        symbolTable.popScope();
    }

    if (lookahead.type == ELSE)
    {
        match(ELSE);
        if (!finishedState)
        {
            string skip = fs->newStateName();
            fs->genJump(skip, lookahead.line);
            fs->genEndState();
            fs->genNewState(fail);
            if (lookahead.type != LBRACE) finishedState = statement(fs);
            else
            {
                match(LBRACE);
                symbolTable.pushScope();
                finishedState = statement(fs);
                while (lookahead.type != RBRACE) finishedState = statement(fs);
                match(RBRACE);
                symbolTable.popScope();
            }

            if (!finishedState)
            {
                fs->genJump(skip, lookahead.line);
                fs->genEndState();
            }
            fs->genNewState(skip);
        }
        else fs->genNewState(fail);
    }
    else
    {
        if (!finishedState)
        {
            fs->genJump(fail, lookahead.line);
            fs->genEndState();
        }
        fs->genNewState(fail);
    }
}

void Compiler::genWhile(FunctionPointer fs)
{
    string loopcheck = fs->newStateName();
    string body = fs->newStateName();
    string end = fs->newStateName();

    fs->genJump(loopcheck, lookahead.line);
    fs->genEndState();
    fs->genNewState(loopcheck);

    match(WHILE);
    match(LPAREN);
    ors(fs, body, end);
    match(RPAREN);

    bool finishedState;
    if (lookahead.type != LBRACE) finishedState = statement(fs);
    else
    {
        match(LBRACE);
        symbolTable.pushScope();
        fs->genNewState(body);
        finishedState = statement(fs);
        while (lookahead.type != RBRACE) finishedState = statement(fs);
        match(RBRACE);
        symbolTable.popScope();
    }
    if (!finishedState)
    {
        fs->genJump(loopcheck, lookahead.line);
        fs->genEndState();
    }
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
    fs->genJump(fail, lookahead.line);
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
    fs->genJump(success, lookahead.line);
    fs->genEndState();
}

void Compiler::condition(FunctionPointer fs, string success, string fail)
{
    expression(fs, "LHS");
    Relations::Relop r = relop();
    expression(fs, "RHS");

    fs->genConditionalJump(success, "LHS", r, "RHS", lookahead.line);
    fs->genJump(fail, lookahead.line);
    fs->genEndState();
}