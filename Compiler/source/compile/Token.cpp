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

namespace Relations
{
    template<typename T>
    bool evaluateRelop(const T& LHS, Relop rel, const T& RHS)
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

    template bool evaluateRelop<double>(double& LHS, Relop rel, double& RHS);
    template bool evaluateRelop<string>(string& LHS, Relop rel, string& RHS);

    Relop mirrorRelop(Relations::Relop rel)
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

    Relop negateRelop(Relations::Relop rel)
    {
        switch (rel)
        {
            case Relations::Relop::EQ:
                return Relations::Relop::NE;
            case Relations::Relop::NE:
                return Relations::Relop::EQ;
            case Relations::Relop::LT:
                return Relations::Relop::GE;
            case Relations::Relop::LE:
                return Relations::Relop::GT;
            case Relations::Relop::GT:
                return Relations::Relop::LE;
            case Relations::Relop::GE:
                return Relations::Relop::LT;
            default:
                throw std::runtime_error("Bad relop encountered");
        }
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