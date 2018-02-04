#include "Token.h"

using namespace std;

string TypeEnumNames[] = {"IDENT", "OP", "RELOP", "LBRACE", "RBRACE",
                          "LPAREN", "RPAREN", "IF", "WHILE",
                          "SEMIC", "FUNCTION", "DTYPE", "CALL",
                          "RETURN", "ASSIGN", "END", "NUMBER",
                          "INPUT", "PRINT", "NOT", "ENDIF", "COMMA",
                          "COMPAND", "COMPOR", "STRINGLIT", "ELSE",
                          "LSQPAREN", "RSQPAREN"};

string VariableTypeEnumNames[] = {"double", "string"};

char opEnumChars[] = {'+', '*', '-', '/', '%', '&', '|'};

double evaluateOp(const double& lhs, ArithOp op, const double& rhs)
{
    switch(op)
    {
        case PLUS:
            return lhs + rhs;
        case MULT:
            return lhs * rhs;
        case MINUS:
            return lhs - rhs;
        case DIV:
            return lhs / rhs;
        case MOD:
            return fmod(lhs, rhs);
        case AND:
            return (int)lhs & int(rhs);
        case OR:
            return (int)lhs | int(rhs);
        default:
            throw std::runtime_error("Unknown op");
    }
}


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
            case NEQ:
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

    template bool evaluateRelop<double>(const double& LHS, Relop rel, const double& RHS);
    template bool evaluateRelop<string>(const string& LHS, Relop rel, const string& RHS);

    Relop mirrorRelop(Relations::Relop rel)
    {
        switch (rel)
        {
            case Relations::Relop::EQ:
                return Relations::Relop::EQ;
            case Relations::Relop::NEQ:
                return Relations::Relop::NEQ;
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
                return Relations::Relop::NEQ;
            case Relations::Relop::NEQ:
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

void Token::setLine(int line)
{
    this->line = line;
}

std::ostream &operator<<(std::ostream &stream, const Token &token)
{
    stream << TypeEnumNames[token.type];
    return stream;
}