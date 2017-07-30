#ifndef COMMAND_H
#define COMMAND_H

#include "compile/Token.h"

enum class CommandSideEffect{NONE, JUMP, CONDJUMP, CHANGEVAR};

class AbstractCommand
{
private:
    std::string data;
    CommandSideEffect effectFlag;

protected:
    void setEffect(CommandSideEffect effect)
    {
        AbstractCommand::effectFlag = effect;
    }

public:

    virtual std::string translation() const = 0;
    virtual ~AbstractCommand() {}

    const std::string &getData() const
    {
        return data;
    }

    CommandSideEffect getEffectFlag() const
    {
        return effectFlag;
    }

    void setData(const std::string &data)
    {
        AbstractCommand::data = data;
    }
};

/*
class AbstractVarEffectCommand : public AbstractCommand
{
public:
    struct VarEffect
    {
        std::string varName;
        bool constrained;
        bool literal;
        Relop relop;
        VariableType type;
        union
        {
            const char* compString; //strings will only be compared to literals, which will not be changed during analysis
            double compDouble;
        };

        VarEffect(std::string varN, Relop rel, double d):
                varName(varN), relop(rel), compDouble(d), type(DOUBLE), constrained(true) {}

        VarEffect(std::string varN, Relop rel, std::string& s, bool literal):
                varName(varN), relop(rel), compString(s.c_str()), type(STRING), constrained(true) {}

        VarEffect(std::string varN): constrained(false) {}

    };

    virtual VarEffect getVarEffect() = 0;
};*/

class PrintCommand: public AbstractCommand
{
public:
    PrintCommand(std::string info)
    {
        setData(info);
        setEffect(CommandSideEffect::NONE);
    }

    std::string translation() const override {return "print " + getData() + ";\n";};
};

class ReturnCommand: public AbstractCommand
{
public:
    ReturnCommand()
    {
        setData("return");
        setEffect(CommandSideEffect::JUMP);
    }
    std::string translation() const override {return "return;\n";}
};

class JumpCommand: public AbstractCommand
{
public:
    JumpCommand(std::string to)
    {
        setData(to);
        setEffect(CommandSideEffect::JUMP);
    }
    std::string translation() const override{return "jump " + getData() + ";\n";};
};

class InputVarCommand: public AbstractCommand
{
public:
    InputVarCommand(std::string assigning)
    {
        setData(assigning);
        setEffect(CommandSideEffect::CHANGEVAR);
    }
    std::string translation() const override{return "input " + getData() + ";\n";};
};

class PushCommand: public AbstractCommand
{
public:
    PushCommand(std::string in)
    {
        setData(in);
        setEffect(CommandSideEffect::NONE);
    }

    std::string translation() const override{return "push " + getData() + ";\n";}

};

class PopCommand: public AbstractCommand
{
public:
    PopCommand(std::string in)
    {
        setData(in);
        setEffect(CommandSideEffect::CHANGEVAR);
    }

    std::string translation() const override{return "pop " + getData() + ";\n";}
};


class AssignVarCommand : public AbstractCommand
{
public:
    std::string RHS;
    AssignVarCommand(std::string lh, std::string rh)
    {
        setData(lh);
        RHS = rh;
        setEffect(CommandSideEffect::CHANGEVAR);
    }

    std::string translation() const override{return getData() + " = " + RHS + ";\n";}
};

class EvaluateExprCommand: public AbstractCommand
{
public:
    std::string term1;
    std::string term2;
    Op op;

    EvaluateExprCommand(std::string lh, std::string t1, Op o, std::string t2)
    {
        setData(lh);
        term1 =t1;
        term2 = t2; 
        op =o;
        setEffect(CommandSideEffect::CHANGEVAR);
    }

    std::string translation() const override{return getData() + " = " + term1 + ' ' + opEnumChars[op]  + ' ' + term2 + ";\n";}
};

class JumpOnComparisonCommand: public AbstractCommand
{
public:
    std::string term1;
    std::string term2;
    Relop op;

    JumpOnComparisonCommand(std::string st, std::string t1, std::string t2, Relop o)
    {
        setData(st);
        term1 = t1;
        term2 = t2; 
        op = o; 
        setEffect(CommandSideEffect::CONDJUMP);
    }

    std::string translation() const override {return "jumpif " + term1 + relEnumStrs[op] + term2 + " " + getData() + ";\n";}
};

class DeclareVariableCommand: public AbstractCommand
{
public:
    VariableType vt;

    DeclareVariableCommand(VariableType t, std::string n)
    {
        vt = t;
        setData(n);
        setEffect(CommandSideEffect::CHANGEVAR);
    }

    std::string translation() const override{return VariableTypeEnumNames[vt] + " " + getData() + ";\n";}
};

#endif