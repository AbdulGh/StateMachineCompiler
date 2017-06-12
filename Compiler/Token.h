#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum Type {IDENT, OP, RELOP, LBRACE, RBRACE,
            LPAREN, RPAREN, IF, WHILE, DO,
            SEMIC, FUNCTION, DTYPE, CALL, THEN,
            RETURN, ASSIGN, END, NUMBER,
            INPUT, PRINT, NOT, ENDIF, DONE, COMMA,
            COMPAND, COMPOR, STRINGLIT};

extern std::string TypeEnumNames[];

enum VariableType {DOUBLE, STRING};

enum Relop {EQ, NE, LT, LE, GT, GE};

enum Op {PLUS, MINUS, MULT, DIV, MOD, AND, OR};

class Token
{
public:
    typedef union at
    {
        Relop relType;
        Op opType;
        VariableType vType;

        at(Relop rel): relType(rel) {};
        at(Op op): opType(op) {};
        at(VariableType vt): vType (vt) {};
        at(): opType{} {};

        operator Relop () const {return relType;}
        operator Op () const {return opType;}
        operator VariableType () const {return vType;}
    } AuxTypeUnion;

    Type type;
    AuxTypeUnion auxType;

    Token(Type t):
            type(t),
            auxType{} {}

    Token(VariableType t):
            type(DTYPE),
            auxType(t) {}

    Token(Relop rel):
            type(RELOP),
            auxType(rel) {}

    Token(Op op):
            type(OP),
            auxType(op) {}

    Token():
            type{},
            auxType{} {}

    friend std::ostream& operator<< (std::ostream& stream, const Token& token);
};

#endif