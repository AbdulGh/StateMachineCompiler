#include "Token.h"

using namespace std;

string TypeEnumNames[] = {"IDENT", "OP", "RELOP", "LBRACE", "RBRACE",
                        "LPAREN", "RPAREN", "IF", "WHILE",
                        "SEMIC", "FUNCTION", "DTYPE", "CALL",
                        "RETURN", "ASSIGN", "END", "NUMBER",
                        "INPUT", "PRINT", "NOT", "ENDIF", "DONE", "COMMA",
                        "COMPAND", "COMPOR", "STRINGLIT", "THEN", "ELSE"};

string VariableTypeEnumNames[] = {"double", "string"};

char opEnumChars[] = {'+', '*','-', '/', '%', '&', '|'};

std::string relEnumStrs[] = {" = ", " != ", " < ", " <= ", " > ", " >= "};

void Token::setLine(unsigned int line)
{
    this->line = line;
}

std::ostream& operator<< (std::ostream& stream, const Token& token)
{
    stream << TypeEnumNames[token.type];
    return stream;
}