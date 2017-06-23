#include "Compiler.h"

using namespace std;

VariableType Compiler::genFunctionCall(VariableType expectedType, shared_ptr<FunctionSymbol> fromFS)
{
    match(Type::CALL);
    string fid = ident();
    FunctionSymbol& toFS = *(findFunction(fid));
    if (expectedType != ANY && !toFS.isOfType(expectedType)) error("Type error");
        //error ("Function '" + fid + "' returns type " + toFS.getReturnType() + ", expected " + expectedType);
    match(Type::LPAREN);

    string nextState = fromFS->genNewStateName();
    fromFS->emit("push state " + nextState + ";\n");

    vector<VariableType> paramTypes;
    while (lookahead.type != Type::RPAREN)
    {
        if (lookahead.type == Type::NUMBER)
        {
            string toPush = lookahead.lexemeString;
            match(Type::NUMBER);
            paramTypes.push_back(VariableType::DOUBLE);
            fromFS->emit("push " + toPush + ";\n");
        }
        else if (lookahead.type == Type::STRINGLIT)
        {
            string toPush = lookahead.lexemeString;
            match(Type::STRINGLIT);
            paramTypes.push_back(VariableType::STRING);
            fromFS->emit("push \"" + toPush + "\";\n");
        }
        else
        {
            string iid = ident();
            shared_ptr<Identifier> idp = findVariable(iid);
            paramTypes.push_back(idp->getType());
            fromFS->emit("push " + idp->getUniqueID() + ";\n");
        }
        if (lookahead.type == Type::COMMA)
        {
            match(Type::COMMA);
            if (lookahead.type == Type::RPAREN) warning("Unexpected comma in arguments for function '" + fid + "' - ignoring");
        }
    }
    match(Type::RPAREN);

    if (!toFS.checkTypes(paramTypes)) error("Type mismatch for function '" + fid + "'");

    fromFS->emit("jump F_" + fid + "_0;\nend\n\n");
    fromFS->emit(nextState + "\n");
    return toFS.getReturnType();
}

void Compiler::genIf(FunctionPointer fs)
{
    string success = fs->genNewStateName();
    string fail = fs->genNewStateName();

    match(IF);
    match(LPAREN);
    ors(fs, success, fail);
    match(RPAREN);

    fs->emit(success +  "\n");
    statement(fs);
    fs->emit("jump " + fail + ";\nend\n\n" + fail + "\n");
}

void Compiler::genWhile(FunctionPointer fs) //todo
{
    match(WHILE);
    match(LPAREN);

    string success = fs->genNewStateName();
    string fail = fs->genNewStateName();
    ors(fs, success, fail);
    match(RPAREN);
    statement(fs);
}

void Compiler::ors(FunctionPointer fs, string success, string fail)
{
    string IM = fs->genNewStateName();
    ands(fs, success, IM);
    while (lookahead.type == COMPOR)
    {
        match(COMPOR);
        fs->emit("end\n\n" + IM + "\n");
        IM = fs->genNewStateName();
        ands(fs, success, IM);
    }
    fs->emit("\n" + IM + "\njump " + fail + ";\nend\n\n");
}

void Compiler::ands(FunctionPointer fs, string success, string fail)
{
    string IM = fs->genNewStateName();
    condition(fs, IM, fail);
    while (lookahead.type == COMPAND)
    {
        fs->emit("end\n\n" + IM + "\n");
        match(COMPAND);
        IM = fs->genNewStateName();
        condition(fs, IM, fail);
    }
    fs->emit("\n" + IM + "\njump " + success + ";\nend\n");
}

void Compiler::condition(FunctionPointer fs, string success, string fail)
{
    expression(fs, "LHS");
    string r = relop();
    expression(fs, "RHS");
    fs->emit("jumpif LHS " + r + " RHS " + success + ";\n");
    fs->emit("jump " + fail + ";\nend\n");
}