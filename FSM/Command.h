#ifndef COMMAND_H
#define COMMAND_H

#include "Variable.h"
#include "State.h"
#include "Enums.h"

class AbstractCommand
{
public:
    //enum SideEffect{PRINT, INPUT, JUMP, SETVAR};

    //SideEffect getType() const;
    int getNextState() const;
    bool changesState() const;

    virtual void execute() = 0;

private:
    int nextState = -1;
    bool changeState = false;
    //SideEffect effect;

protected:
    void setState(int);
    void setChangeState(bool);
};

template <class T>
class PrintCommand: public AbstractCommand
{
public:
    PrintCommand(T);
    void execute();
private:
    T toPrint;
};

class JumpCommand: public AbstractCommand
{
public:
    JumpCommand(int);
    void execute();
};

class InputVarCommand: public AbstractCommand
{
public:
    InputVarCommand(std::shared_ptr<Variable> varPtr);
    void execute();
private:
    std::shared_ptr<Variable> var;
};

template <class T>
class AssignVarCommand: public AbstractCommand
{
public:
    AssignVarCommand(std::shared_ptr<Variable> varPtr, T value);
    void execute();
private:
    std::shared_ptr<Variable> var;
    T val;
};

template <class T>
class EvaluateExprCommand: public AbstractCommand
{
public:
    EvaluateExprCommand(std::shared_ptr<Variable> varPtr, std::shared_ptr<Variable> RHSVar, T b, ExpressionType t);
    void execute();
private:
    void evaluate(double one, double two);
    std::shared_ptr<Variable> var;
    ExpressionType type;
    std::shared_ptr<Variable> term1;
    T term2;
};

template <typename T>
class JumpOnComparisonCommand: public AbstractCommand
{
public:
    JumpOnComparisonCommand(std::shared_ptr<Variable> varPtr, T compareTo, int state, ComparisonOp type);
    void execute();
private:
    void evaluate(double RHS);
    std::shared_ptr<Variable> var;
    T compareTo;
    ComparisonOp cop;
};

#endif