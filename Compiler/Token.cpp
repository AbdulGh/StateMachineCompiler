#include "Token.h"

using namespace std;

string TypeEnumNames[] = {"IDENT", "OP", "RELOP", "LBRACE", "RBRACE",
                        "LPAREN", "RPAREN", "IF", "THEN",
                        "SEMIC", "FUNCTION", "DTYPE", "CALL",
                        "RETURN", "EVEN", "ASSI", "END", "QUOTE"};

std::ostream& operator<< (std::ostream& stream, const Token& token)
{
    stream << TypeEnumNames[token.type];
    return stream;
}