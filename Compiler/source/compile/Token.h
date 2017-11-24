#ifndef PROJECT_TOKEN_H
#define PROJECT_TOKEN_H

#include <string>
#include <stdexcept>
#include <cmath>

enum Type {IDENT, OP, RELOP, LBRACE, RBRACE,
            LPAREN, RPAREN, IF, WHILE,
            SEMIC, FUNCTION, DTYPE, CALL,
            RETURN, ASSIGN, END, NUMBER,
            INPUT, PRINT, NOT, ENDIF, COMMA,
            COMPAND, COMPOR, STRINGLIT, ELSE};

extern std::string TypeEnumNames[];

enum VariableType {DOUBLE, STRING, VOID, ANY}; //any never generated by lexer

extern std::string VariableTypeEnumNames[];

namespace Relations
{
    enum Relop{EQ, NEQ, LT, LE, GT, GE};

    template <typename T>
    bool evaluateRelop(const T& LHS, Relop rel, const T& RHS);
    Relop mirrorRelop(Relop rel);
    Relop negateRelop(Relop rel);
}
extern std::string relEnumStrs[];

enum Op {PLUS, MULT, MINUS, DIV, MOD, AND, OR};
double evaluateOp(const double& lhs, Op op, const double& rhs);
extern char opEnumChars[];

class Token
{
public:
    typedef union at
    {
        Relations::Relop relType;
        Op opType;
        VariableType vType;

        at(Relations::Relop rel): relType(rel) {};
        at(Op op): opType(op) {};
        at(VariableType vt): vType (vt) {};
        at(): opType{} {};

        operator Relations::Relop () const {return relType;}
        operator Op () const {return opType;}
        operator VariableType () const {return vType;}
    } AuxTypeUnion;

    Type type;
    AuxTypeUnion auxType;
    unsigned int line;
    std::string lexemeString;

    Token(Type t, std::string s = ""):
            type(t),
            auxType{},
            lexemeString(move(s)){}

    Token(VariableType t, std::string s = ""):
            type(DTYPE),
            auxType(t),
            lexemeString(move(s)){}

    Token(Relations::Relop rel):
            type(RELOP),
            auxType(rel)
    {
        switch((Relations::Relop)auxType)
        {
            case Relations::EQ:
                lexemeString = "==";
                break;
            case Relations::NEQ:
                lexemeString = "!=";
                break;
            case Relations::LT:
                lexemeString = "<";
                break;
            case Relations::LE:
                lexemeString = "<=";
                break;
            case Relations::GT:
                lexemeString = ">";
                break;
            case Relations::GE:
                lexemeString = ">=";
                break;
        }
    }

    Token(Op op):
            type(OP),
            auxType(op) {}

    Token():
            type{},
            auxType{} {}

    void setLine(int);

    friend std::ostream& operator<< (std::ostream& stream, const Token& token);
};

#endif