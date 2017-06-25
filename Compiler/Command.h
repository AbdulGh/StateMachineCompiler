#ifndef COMMAND_H
#define COMMAND_H

#include "Token.h"

class AbstractCommand
{
public:
    //enum SideEffect{PRINT, JUMP, CHANGEVAR};
    virtual std::string translation() const = 0;
    //enum DataType{NUM, STRINGLIT, VAR};
};

class PrintCommand: public AbstractCommand
{
public:
    //DataType type;
    std::string toPrint;

    PrintCommand(std::string info):
             toPrint(info){}

    std::string translation() const override {return "print " + toPrint + ";\n";};
};

class JumpCommand: public AbstractCommand
{
public:
    std::string toState;

    JumpCommand(std::string to): toState(to){}
    std::string translation() const override{return "jump " + toState + ";\n";};
};

class InputVarCommand: public AbstractCommand
{
public:
    std::string toVar;
    InputVarCommand(std::string assigning): toVar(assigning){}
    std::string translation() const override{return "input " + toVar + ";\n";};
};

class PushCommand: public AbstractCommand
{
public:
    std::string toPush;
    //DataType type;
    PushCommand(std::string in):
            toPush(in){}

    std::string translation() const override{return "push " + toPush + ";\n";}

};

class PopCommand: public AbstractCommand
{
public:
    std::string toPop;
    //DataType type;
    PopCommand(std::string in):
    /*type(t),*/ toPop(in){}

    std::string translation() const override{return "push " + toPop + ";\n";}
};


class AssignVarCommand: public AbstractCommand
{
public:
    std::string LHS;
    std::string RHS;
    //DataType type;
    AssignVarCommand(std::string lh, std::string rh):
    /*type(t),*/ LHS(lh), RHS(rh){}

    std::string translation() const override{return LHS + " = " + RHS + ";\n";}
};

class EvaluateExprCommand: public AbstractCommand
{
public:
    //enum Op{MULT, DIV, PLUS, MINUS, MOD};

    std::string LHS;
    std::string term1;
    std::string term2;
    Op op;

    EvaluateExprCommand(std::string lh, std::string t1, Op o, std::string t2):
    LHS(lh), term1(t1), term2(t2), op(o){}

    std::string translation() const override{return LHS + " = " + term1 + ' ' + opEnumChars[op]  + ' ' + term2 + ";\n";}
};


class JumpOnComparisonCommand: public AbstractCommand
{
public:
    //enum Rel{LE, LT, GE, GT, EQ, NEQ};

    std::string state;
    std::string term1;
    std::string term2;
    Relop op;

    JumpOnComparisonCommand(std::string st, std::string t1, std::string t2, Relop o):
    state(st), term1(t1), term2(t2), op(o) {}

    std::string translation() const override {return "jumpif " + term1 + relEnumStrs[op] + term2 + " " + state + ";\n";}
};

class DeclareVariableCommand: public AbstractCommand
{
public:
    VariableType vt;
    std::string id;

    DeclareVariableCommand(VariableType t, std::string n):
            vt(t), id(n){}

    std::string translation() const override{return VariableTypeEnumNames[vt] + " " + id + ";\n";}
};

#endif