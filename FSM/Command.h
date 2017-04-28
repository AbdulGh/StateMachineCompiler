#ifndef COMMAND_H
#define COMMAND_H

#include "Variable.h"

class AbstractCommand
{
public:
    enum SideEffect{PRINT, INPUT, JUMP, SETVAR};

    SideEffect getType() const;
    int getNextState() const;
    bool changesState() const;

    virtual void execute() = 0;

protected:
    int nextState = -1;
    bool changeState = false;
    SideEffect effect;
};

class PrintConstCommand: public AbstractCommand
{
public:
    PrintConstCommand(std::string);
    void execute();
private:
    std::string str;
    SideEffect effect;
};

class PrintVarCommand: public AbstractCommand
{
public:
    PrintVarCommand(std::shared_ptr<Variable> varPtr);
    void execute();
private:
    std::shared_ptr<Variable> var;
    SideEffect effect;

};

class JumpCommand: public AbstractCommand
{
public:
    JumpCommand(int);
    void execute();
private:
    SideEffect effect;
};


class InputVarCommand: public AbstractCommand
{
public:
    InputVarCommand(std::shared_ptr<Variable> varPtr);
    void execute();
private:
    std::shared_ptr<Variable> var;
    SideEffect effect;
};

class JumpOnComparisonCommand: public AbstractCommand
{
public:
    enum ComparisonOp{GT, GE, LT, LE, EQ, NEQ};
    enum ComparingType{INT, DOUBLE, VAR};
    JumpOnComparisonCommand(std::shared_ptr<Variable> varPtr, int constInt, int state, JumpOnComparisonCommand::ComparisonOp type);
    JumpOnComparisonCommand(std::shared_ptr<Variable> varPtr, double constDouble, int state, JumpOnComparisonCommand::ComparisonOp type);
    JumpOnComparisonCommand(std::shared_ptr<Variable> varPtr1, std::shared_ptr<Variable> varPtr2, int state, JumpOnComparisonCommand::ComparisonOp type);
    void execute();

    typedef union CT
    {
        int i;
        double d;
        std::shared_ptr<Variable> var;

        CT(int in): i(in) {}
        CT(double in): d(in) {}
        CT(std::shared_ptr<Variable> in): var(in) {}
    } CompareUnion;
private:
    std::shared_ptr<Variable> var;
    CompareUnion compareTo;
    ComparingType ctype;
    SideEffect effect;
    JumpOnComparisonCommand::ComparisonOp cop;
};


#endif