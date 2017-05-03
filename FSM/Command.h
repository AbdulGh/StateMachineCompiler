#ifndef COMMAND_H
#define COMMAND_H

#include "Variable.h"
#include "Enums.h"

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
    int nextState;
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

template <class T>
class AssignVarCommand: public AbstractCommand
{
public:
    AssignVarCommand(std::shared_ptr<Variable> varPtr, T value);
    void execute();
private:
    std::shared_ptr<Variable> var;
    SideEffect effect;
    T val;
};

template <typename T>
class JumpOnComparisonCommand: public AbstractCommand
{
public:
    JumpOnComparisonCommand(std::shared_ptr<Variable> varPtr, T compareTo, int state, ComparisonOp type);
    void execute();
private:
    std::shared_ptr<Variable> var;
    T compareTo;
    SideEffect effect;
    ComparisonOp cop;
    int nextState;
};

#endif