#include "Compiler.h"
#include "../symbolic/VarWrappers.h"

using namespace std;
void Compiler::genFunctionCall(FunctionSymbol* fromFS, VariableType expectedType, unique_ptr<VarWrapper> uid)
{
    match(Type::CALL);
    string fid = plainIdent();
    FunctionSymbol *toFS = functionTable.getFunction(fid);

    if (expectedType != ANY && !toFS->isOfType(expectedType))
    {
        error("Function '" + fid + "' returns type " + TypeEnumNames[toFS->getReturnType()]
              + ", expected " + TypeEnumNames[expectedType]);
    }

    //push all vars
    const set<VarWrapper*>& fromVars = fromFS->getVars(); //sets are ordered
    for (auto& s : fromVars) fromFS->genPush(s->clone(), lookahead.line);

    string nextState = fromFS->newStateName();
    fromFS->genPush(nextState, lookahead.line, toFS);

    match(Type::LPAREN);
    if (lookahead.type != Type::RPAREN)
    {
        vector<VariableType> paramTypes;
        do
        {
            if (lookahead.type == Type::NUMBER)
            {
                string toPush = lookahead.lexemeString;
                match(Type::NUMBER);
                paramTypes.push_back(VariableType::DOUBLE);
                fromFS->genPush(toPush, lookahead.line);
            }
            else if (lookahead.type == Type::STRINGLIT)
            {
                string toPush = lookahead.lexemeString;
                match(Type::STRINGLIT);
                paramTypes.push_back(VariableType::STRING);
                fromFS->genPush(quoteString(toPush), lookahead.line);
            }
            else
            {
                Identifier* id;
                unique_ptr<VarWrapper> vg = wrappedIdent(&id);
                VariableType type = id->getType();
                paramTypes.push_back(type == ARRAY ? DOUBLE : type);
                fromFS->genPush(move(vg), lookahead.line);
            }
            if (lookahead.type == Type::COMMA)
            {
                match(Type::COMMA);
                if (lookahead.type == Type::RPAREN)
                    warning("Unexpected comma in arguments for function '" + fid + "' - ignoring");
            }
        } while (lookahead.type != Type::RPAREN);

        if (!toFS->checkTypes(paramTypes)) error("Type mismatch in parameters for function '" + fid + "'");
    }
    match(Type::RPAREN);

    fromFS->genJump(toFS->getFirstNode()->getName(), lookahead.line);
    CFGNode* finishedState = fromFS->getCurrentNode();
    cfg.createNode(nextState, true, false, fromFS); //forward create returned to state
    fromFS->genEndState();
    fromFS->genNewState(nextState);
    CFGNode* created = fromFS->getCurrentNode();
    toFS->addFunctionCall(finishedState, created, fromVars.size());
    finishedState->setFunctionCall(toFS);
    created->addFunctionCall(finishedState, toFS);
    created->addParent(toFS->getLastNode());

    //pop all vars back
    for (auto rit = fromVars.rbegin(); rit != fromVars.rend(); ++rit) fromFS->genPop((*rit)->clone(), lookahead.line);

    if (uid)
    {
        switch (toFS->getReturnType())
        {
            case DOUBLE:
                fromFS->genAssignment(move(uid), Atom("retD"), lookahead.line);
                break;
            case STRING:
                fromFS->genAssignment(move(uid), Atom("retS"), lookahead.line);
                break;
            default:
                throw runtime_error("Unaccounted for variable type");
        }
    }
}

void Compiler::genIf(FunctionSymbol* fs)
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
        fs->pushScope();
        finishedState = statement(fs);
        while (lookahead.type != RBRACE) finishedState = statement(fs);
        match(RBRACE);
        fs->popScope();
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
                fs->pushScope();
                finishedState = statement(fs);
                while (lookahead.type != RBRACE) finishedState = statement(fs);
                match(RBRACE);
                fs->popScope();
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

void Compiler::genWhile(FunctionSymbol* fs)
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

    fs->genNewState(body);
    bool finishedState;
    if (lookahead.type != LBRACE) finishedState = statement(fs);
    else
    {
        match(LBRACE);
        symbolTable.pushScope();
        fs->pushScope();
        finishedState = statement(fs);
        while (lookahead.type != RBRACE) finishedState = statement(fs);
        match(RBRACE);
        fs->popScope();
        symbolTable.popScope();
    }
    if (!finishedState)
    {
        fs->genJump(loopcheck, lookahead.line);
        fs->genEndState();
    }
    fs->genNewState(end);
}

void Compiler::ors(FunctionSymbol* fs, string success, string fail)
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

void Compiler::ands(FunctionSymbol* fs, string success, string fail)
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

void Compiler::condition(FunctionSymbol* fs, string success, string fail)
{
    expression(fs, make_unique<SVByName>("LHS"));
    Relations::Relop r = relop();
    expression(fs, make_unique<SVByName>("RHS"));

    fs->genConditionalJump(move(success), make_unique<SVByName>("LHS"), r, make_unique<SVByName>("RHS"), lookahead.line);
    fs->genJump(move(fail), lookahead.line);
    fs->genEndState();
}