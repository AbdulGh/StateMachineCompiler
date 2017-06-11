#include "Token.h"

using namespace std;

string TypeEnumNames[] = {"IDENT", "OP", "RELOP", "LBRACE", "RBRACE",
                        "LPAREN", "RPAREN", "IF", "WHILE", "DO",
                        "SEMIC", "FUNCTION", "DTYPE", "CALL", "THEN",
                        "RETURN", "ASSIGN", "END", "QUOTE", "NUMBER",
                        "INPUT", "PRINT", "NOT", "ENDIF", "DONE", "COMMA",
                        "COMPAND", "COMPOR"};

std::ostream& operator<< (std::ostream& stream, const Token& token)
{
    stream << TypeEnumNames[token.type];
    return stream;
}