#ifndef COMMAND_H
#define COMMAND_H

#include <stack>

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

template <typename T>
class PrintCommand: public AbstractCommand
{
public:
    PrintCommand(T);
    void execute();
private:
    static std::string unescape(const std::string&);
    T toPrint;
};

class JumpCommand: public AbstractCommand
{
public:
    JumpCommand(int);
    void execute();
};

class JumpTopCommand: public AbstractCommand
{
public:
    JumpTopCommand(FSM& stackOwner);
    void execute();
private:
    std::stack<Variable::TaggedDataUnion>* popFrom;
};

class InputVarCommand: public AbstractCommand
{
public:
    InputVarCommand(std::shared_ptr<Variable> varPtr);
    void execute();
private:
    std::shared_ptr<Variable> var;
};

template <typename T>
class PushCommand: public AbstractCommand
{
public:
    PushCommand(T in, FSM& stackOwner);
    void execute();
private:
    T var;
    std::stack<Variable::TaggedDataUnion>* pushTo;
};

class PopCommand: public AbstractCommand
{
public:
    PopCommand(std::shared_ptr<Variable> varPtr, FSM& stackOwner);
    void execute();
private:
    std::shared_ptr<Variable> var;
    std::stack<Variable::TaggedDataUnion>* popFrom;
};

template <typename T>
class AssignVarCommand: public AbstractCommand
{
public:
    AssignVarCommand(std::shared_ptr<Variable> varPtr, T value);
    void execute();
private:
    std::shared_ptr<Variable> var;
    T val;
};

template <typename T>
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
    JumpOnComparisonCommand(std::shared_ptr<Variable> varPtr, T compareTo, FSM& stackOwner, ComparisonOp type);
    void execute();
private:
    void evaluate(double RHS);
    void evaluate(std::string RHS);
    std::shared_ptr<Variable> var;
    T compareTo;
    ComparisonOp cop;
    std::stack<Variable::TaggedDataUnion>* popFrom;
};

#endif