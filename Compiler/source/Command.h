#ifndef COMMAND_H
#define COMMAND_H

#include "compile/Token.h"

enum class CommandSideEffect{NONE, JUMP, CONDJUMP, CHANGEVAR};

namespace SymbolicExecution {class SymbolicExecutionFringe;}; //symbolic/SymbolicExecution.cpp
class AbstractCommand
{
private:
    std::string data;
    CommandSideEffect effectFlag;
    int linenumber;

protected:
    void setEffect(CommandSideEffect effect)
    {
        AbstractCommand::effectFlag = effect;
    }

public:

    virtual std::string translation() const = 0;
    AbstractCommand() {}
    AbstractCommand(int line): linenumber(line) {}
    virtual ~AbstractCommand() {}

    //returns if the symbolic execution of this command went through
    virtual bool acceptSymbolicExecution(SymbolicExecution::SymbolicExecutionFringe* svs);

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

    int getLineNum()
    {
        return linenumber;
    }
};

class PrintCommand: public AbstractCommand
{
public:
    PrintCommand(std::string info, int linenum) : AbstractCommand(linenum)
    {
        setData(info);
        setEffect(CommandSideEffect::NONE);
    }

    std::string translation() const override {return "print " + getData() + ";\n";};
};

class ReturnCommand: public AbstractCommand
{
public:
    ReturnCommand(int linenum) : AbstractCommand(linenum)
    {
        setData("return");
        setEffect(CommandSideEffect::JUMP);
    }
    std::string translation() const override {return "return;\n";}
};

class JumpCommand: public AbstractCommand
{
public:
    JumpCommand(std::string to, int linenum) : AbstractCommand(linenum)
    {
        setData(to);
        setEffect(CommandSideEffect::JUMP);
    }
    std::string translation() const override{return "jump " + getData() + ";\n";};
};

class JumpOnComparisonCommand: public AbstractCommand
{
public:
    std::string term1;
    std::string term2;
    Relop op;

    JumpOnComparisonCommand(std::string st, std::string t1, std::string t2, Relop o, int linenum) : AbstractCommand(linenum)
    {
        setData(st);
        term1 = t1;
        term2 = t2;
        op = o;
        setEffect(CommandSideEffect::CONDJUMP);
    }

    std::string translation() const override {return "jumpif " + term1 + relEnumStrs[op] + term2 + " " + getData() + ";\n";}
};

class InputVarCommand: public AbstractCommand
{
public:
    InputVarCommand(std::string assigning, int linenum) : AbstractCommand(linenum)
    {
        setData(assigning);
        setEffect(CommandSideEffect::CHANGEVAR);
    }
    std::string translation() const override{return "input " + getData() + ";\n";};
    bool acceptSymbolicExecution(SymbolicExecution::SymbolicExecutionFringe* svs) override;
};

class PushCommand: public AbstractCommand
{
public:
    PushCommand(std::string in, int linenum) : AbstractCommand(linenum)
    {
        setData(in);
        setEffect(CommandSideEffect::NONE);
    }

    std::string translation() const override{return "push " + getData() + ";\n";}
    bool acceptSymbolicExecution(SymbolicExecution::SymbolicExecutionFringe* svs) override;
};

class PopCommand: public AbstractCommand
{
public:
    PopCommand(std::string in, int linenum) : AbstractCommand(linenum)
    {
        setData(in);
        setEffect(CommandSideEffect::CHANGEVAR);
    }

    std::string translation() const override{return "pop " + getData() + ";\n";}
    bool acceptSymbolicExecution(SymbolicExecution::SymbolicExecutionFringe* svs) override;
};


class AssignVarCommand: public AbstractCommand
{
public:
    std::string RHS;
    AssignVarCommand(std::string lh, std::string rh, int linenum) : AbstractCommand(linenum)
    {
        setData(lh);
        RHS = rh;
        setEffect(CommandSideEffect::CHANGEVAR);
    }

    std::string translation() const override{return getData() + " = " + RHS + ";\n";}
    bool acceptSymbolicExecution(SymbolicExecution::SymbolicExecutionFringe* svs) override;
};

class EvaluateExprCommand: public AbstractCommand
{
public:
    std::string term1;
    std::string term2;
    Op op;

    EvaluateExprCommand(std::string lh, std::string t1, Op o, std::string t2, int linenum) : AbstractCommand(linenum)
    {
        setData(lh);
        term1 = t1;
        term2 = t2; 
        op =o;
        setEffect(CommandSideEffect::CHANGEVAR);
    }

    std::string translation() const override{return getData() + " = " + term1 + ' ' + opEnumChars[op]  + ' ' + term2 + ";\n";}
    bool acceptSymbolicExecution(SymbolicExecution::SymbolicExecutionFringe* svs) override;
};

class DeclareVarCommand: public AbstractCommand
{
public:
    VariableType vt;

    DeclareVarCommand(VariableType t, std::string n, int linenum) : AbstractCommand(linenum)
    {
        vt = t;
        setData(n);
        setEffect(CommandSideEffect::CHANGEVAR);
    }

    std::string translation() const override{return VariableTypeEnumNames[vt] + " " + getData() + ";\n";}
    bool acceptSymbolicExecution(SymbolicExecution::SymbolicExecutionFringe* svs) override;
};

#endif