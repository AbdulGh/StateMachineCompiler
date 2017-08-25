#ifndef PROJECT_TOKEN_H
#define PROJECT_TOKEN_H

#include <string>
#include <stdexcept>

enum Type {IDENT, OP, RELOP, LBRACE, RBRACE,
            LPAREN, RPAREN, IF, WHILE,
            SEMIC, FUNCTION, DTYPE, CALL,
            RETURN, ASSIGN, END, NUMBER,
            INPUT, PRINT, NOT, ENDIF, DONE, COMMA,
            COMPAND, COMPOR, STRINGLIT,
            THEN, ELSE};

extern std::string TypeEnumNames[];

enum VariableType {DOUBLE, STRING, VOID, ANY}; //any never generated by lexer

extern std::string VariableTypeEnumNames[];


namespace Relations
{
    enum Relop{EQ, NE, LT, LE, GT, GE};

    template <typename T>
    bool evaluateRelop(T LHS, Relop rel, T RHS);
    Relop mirrorRelop(Relop rel);
}
extern std::string relEnumStrs[];

enum Op {PLUS, MULT, MINUS, DIV, MOD, AND, OR};

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
            lexemeString(s){}

    Token(VariableType t, std::string s = ""):
            type(DTYPE),
            auxType(t),
            lexemeString(s){}

    Token(Relations::Relop rel):
            type(RELOP),
            auxType(rel)
    {
        switch((Relations::Relop)auxType)
        {
            case Relations::EQ:
                lexemeString = "==";
                break;
            case Relations::NE:
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

    void setLine(unsigned int);

    friend std::ostream& operator<< (std::ostream& stream, const Token& token);
};

#endif