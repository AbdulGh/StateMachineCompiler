#ifndef COMMAND_H
#define COMMAND_H

#include "ScopeManager.h"

class AbstractCommand
{
public:
    enum Type{PRINT, JUMP};

    Type getType() const;
    int getNextState() const;
    bool changesState() const;

    virtual void execute(ScopeManager) = 0;

protected:
    int nextState = -1;
    bool changeState = false;
    Type type;
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
    PrintVarCommand(std::string); //todo make this grab the variable itself
    void execute(ScopeManager);
private:
    std::string varN;
    Variable::Type varType;

};

class JumpCommand: public AbstractCommand
{
public:
    JumpCommand(int);
    void execute(ScopeManager);
private:
    int state;

};


#endif