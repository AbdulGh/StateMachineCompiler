#ifndef COMMAND_H
#define COMMAND_H

#include "ScopeManager.h"

class AbstractCommand
{
public:
    enum SideEffect{PRINT, INPUT, JUMP, NONE};

    SideEffect getType() const;
    int getNextState() const;
    bool changesState() const;

    virtual void execute(ScopeManager) = 0;

protected:
    int nextState = -1;
    bool changeState = false;
    SideEffect effect;
};

class PrintConstCommand: public AbstractCommand
{
public:
    PrintConstCommand(std::string);
    void execute(ScopeManager);
private:
    std::string str;
};

class PrintVarCommand: public AbstractCommand
{
public:
    PrintVarCommand(std::string varN);
    void execute(ScopeManager);
private:
    std::string varN;

};

class JumpCommand: public AbstractCommand
{
public:
    JumpCommand(int);
    void execute(ScopeManager);
private:
    int state;
};

class DeclareVarCommand: public AbstractCommand
{
public:
    DeclareVarCommand(Variable::Type, std::string);
    void execute(ScopeManager);
private:
    Variable::Type type;
    std::string name;
};

class InputVarCommand: public AbstractCommand
{
public:
    InputVarCommand(std::string name);
    void execute(ScopeManager);
private:
    std::string varN;

};


#endif