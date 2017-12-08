#ifndef COMMAND_H
#define COMMAND_H

#include <stack>

#include "Variable.h"
#include "State.h"
#include "Enums.h"

class AbstractCommand
{
public:
    int getNextState() const;
    bool changesState() const;

    virtual void execute() = 0;

private:
    int nextState = -1;
    bool changeState = false;

protected:
    void setState(int);
    void setChangeState(bool);
};

template <typename T>
class PrintCommand: public AbstractCommand
{
public:
    PrintCommand(T);
    void execute() override;
private:
    static std::string unescape(const std::string&);
    T toPrint;
};

class JumpCommand: public AbstractCommand
{
public:
    JumpCommand(int);
    void execute() override;
};

class ReturnCommand: public AbstractCommand
{
public:
    ReturnCommand(FSM& stackOwner);
    void execute() override;
private:
    std::stack<Variable::TaggedDataUnion>* popFrom;
};

class InputVarCommand: public AbstractCommand
{
public:
    InputVarCommand(Variable* varPtr);
    void execute() override;
private:
    Variable* var;
};

template <typename T>
class PushCommand: public AbstractCommand
{
public:
    PushCommand(T in, FSM& stackOwner);
    void execute() override;
private:
    T var;
    std::stack<Variable::TaggedDataUnion>* pushTo;
};

class PopCommand: public AbstractCommand
{
public:
    PopCommand(Variable* varPtr, FSM& stackOwner);
    void execute() override;
private:
    Variable* var;
    std::stack<Variable::TaggedDataUnion>* popFrom;
};

template <typename T>
class AssignVarCommand: public AbstractCommand
{
public:
    AssignVarCommand(Variable* varPtr, T value);
    void execute() override;
private:
    Variable* var;
    T val;
};

template <typename T>
class EvaluateExprCommand: public AbstractCommand
{
public:
    EvaluateExprCommand(Variable* varPtr, Variable* RHSVar, T b, ExpressionType t);
    void execute() override;
private:
    void evaluate(double one, double two);
    Variable* var;
    ExpressionType type;
    Variable* term1;
    T term2;
};

template <typename T>
class JumpOnComparisonCommand: public AbstractCommand
{
public:
    JumpOnComparisonCommand(Variable* varPtr, T compareTo, int state, ComparisonOp type);
    JumpOnComparisonCommand(Variable* varPtr, T compareTo, FSM& stackOwner, ComparisonOp type);
    void execute() override;
private:
    Variable* var;
    T compareTo;
    ComparisonOp cop;
    std::stack<Variable::TaggedDataUnion>* popFrom;
};

#endif