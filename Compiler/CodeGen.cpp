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
            if (lookahead.type == Type::RPAREN) error("Unexpected comma"); //todo warnings
        }
    }
    match(Type::RPAREN);

    if (!toFS.checkTypes(paramTypes)) error("Type mismatch for function '" + fid + "'");

    fromFS->emit("jump F_" + fid + "_0;\nend\n\n");
    fromFS->emit(nextState + "\n");
    return toFS.getReturnType();
}