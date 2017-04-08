#ifndef COMMAND_H
#define COMMAND_H

#include "VariableManager.h"

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

class JumpOnConstComparisonCommand: public AbstractCommand
{
public:
    enum ComparisonType{GT, GE, LT, LE, EQ, NEQ};
    JumpOnConstComparisonCommand(std::shared_ptr<Variable> varPtr, int constInt, int state, JumpOnConstComparisonCommand::ComparisonType type);
    void execute();
private:
    std::shared_ptr<Variable> var;
    int compareTo;
    SideEffect effect;
    JumpOnConstComparisonCommand::ComparisonType ctype;
};


#endif