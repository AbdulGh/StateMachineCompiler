#include "Token.h"

using namespace std;

string TypeEnumNames[] = {"IDENT", "OP", "RELOP", "LBRACE", "RBRACE",
                          "LPAREN", "RPAREN", "IF", "WHILE",
                          "SEMIC", "FUNCTION", "DTYPE", "CALL",
                          "RETURN", "ASSIGN", "END", "NUMBER",
                          "INPUT", "PRINT", "NOT", "ENDIF", "COMMA",
                          "COMPAND", "COMPOR", "STRINGLIT", "ELSE"};

string VariableTypeEnumNames[] = {"double", "string"};

char opEnumChars[] = {'+', '*', '-', '/', '%', '&', '|'};

std::string relEnumStrs[] = {" = ", " != ", " < ", " <= ", " > ", " >= "};

template<typename T>
bool Relations::evaluateRelop(T LHS, Relations::Relop rel, T RHS)
{
    switch (rel)
    {
        case EQ:
            return LHS == RHS;
        case NE:
            return LHS != RHS;
        case LT:
            return LHS < RHS;
        case LE:
            return LHS <= RHS;
        case GT:
            return LHS > RHS;
        case GE:
            return LHS >= RHS;
        default:
            throw std::runtime_error("Bad relop encountered");
    }
}

template bool Relations::evaluateRelop<double>(double LHS, Relations::Relop rel, double RHS);
template bool Relations::evaluateRelop<string>(string LHS, Relations::Relop rel, string RHS);

Relations::Relop mirrorRelop(Relations::Relop rel)
{
    switch (rel)
    {
        case Relations::Relop::EQ:
            return Relations::Relop::EQ;
        case Relations::Relop::NE:
            return Relations::Relop::NE;
        case Relations::Relop::LT:
            return Relations::Relop::GT;
        case Relations::Relop::LE:
            return Relations::Relop::GE;
        case Relations::Relop::GT:
            return Relations::Relop::LT;
        case Relations::Relop::GE:
            return Relations::Relop::LE;
        default:
            throw std::runtime_error("Bad relop encountered");
    }
}

void Token::setLine(unsigned int line)
{
    this->line = line;
}

std::ostream &operator<<(std::ostream &stream, const Token &token)
{
    stream << TypeEnumNames[token.type];
    return stream;
}