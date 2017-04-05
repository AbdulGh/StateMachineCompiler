#include <iostream>

#include "Command.h"

using namespace std;

AbstractCommand::SideEffect AbstractCommand::getType() const
{
    return effect;
}

int AbstractCommand::getNextState() const
{
    return nextState;
}

bool AbstractCommand::changesState() const
{
    return changeState;
}

/*PrintConstCommand*/
PrintConstCommand::PrintConstCommand(string istr)
{
    str = istr;
    effect = PRINT;
}

void PrintConstCommand::execute(ScopeManager sm)
{
    cout << str;
}

/*PrintVarCommand*/
PrintVarCommand::PrintVarCommand(std::string name)
{
    varN = name;
    effect = PRINT;
}

void PrintVarCommand::execute(ScopeManager sm)
{
    shared_ptr<Variable> var = sm.findVariable(varN);
    switch(var->getType())
    {
        case Variable::Type::STRING:
            cout << (char*) var->getData();
            break;
        case Variable::Type::INT:
            cout << (int) var->getData();
            break;
        case Variable::Type::DOUBLE:
            cout << (double) var->getData();
            break;
    }
}

/*JumpCommand*/
JumpCommand::JumpCommand(int state)
{
    effect = JUMP;
    changeState = true;
    nextState = state;
}

void JumpCommand::execute(ScopeManager){}

/*DeclareVarCommand*/
DeclareVarCommand::DeclareVarCommand(Variable::Type t, std::string s):
    type(t),
    name(s),
    effect(SETVAR)
{}

void DeclareVarCommand::execute(ScopeManager sm) {sm.declare(name, type);}

InputVarCommand::InputVarCommand(string name):
        varN(name),
        effect(INPUT)
{}

void InputVarCommand::execute(ScopeManager sm)
{
    shared_ptr<Variable> var = sm.findVariable(varN);
    switch(var->getType())
    {
        case Variable::Type::STRING:
        {
            string str;
            cin >> str;
            var->setData(str);
            break;
        }
        case Variable::Type::INT:
        {
            int i;
            cin >> i;
            var->setData(i);
            break;
        }
        case Variable::Type::DOUBLE:
        {
            int d;
            cin >> d;
            var->setData(d);
            break;
        }
    }
}