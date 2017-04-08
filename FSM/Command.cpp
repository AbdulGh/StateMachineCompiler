#include <iostream>

#include "Command.h"
#include "VariableManager.h"

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

void PrintConstCommand::execute()
{
    cout << str;
}

/*PrintVarCommand*/
PrintVarCommand::PrintVarCommand(std::shared_ptr<Variable> varPtr)
{
    var = varPtr;
    effect = PRINT;
}

void PrintVarCommand::execute()
{
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

void JumpCommand::execute(){}


/*InputVarCommand*/
InputVarCommand::InputVarCommand(std::shared_ptr<Variable> varPtr):
        var(varPtr),
        effect(INPUT)
{}

void InputVarCommand::execute()
{
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

/*JumpOnConstComparisonCommand*/
JumpOnConstComparisonCommand::JumpOnConstComparisonCommand(std::shared_ptr<Variable> varPtr, int constInt, int jstate,
                                                           JumpOnConstComparisonCommand::ComparisonType type)
{
    if (var->getType() != Variable::Type::INT) throw "Cannot compare this var.";
    effect = JUMP;
    ctype = type;
    var = varPtr;
    nextState = jstate;
    compareTo = constInt;
}

void JumpOnConstComparisonCommand::execute()
{
    int data = (int)var->getData();

    switch (ctype)
    {
        case GT:
            changeState = data > compareTo;
            break;
        case GE:
            changeState = data >= compareTo;
            break;
        case LT:
            changeState = data < compareTo;
            break;
        case LE:
            changeState = data <= compareTo;
            break;
        case EQ:
            changeState = data == compareTo;
            break;
        case NEQ:
            changeState = data != compareTo;
            break;
    }
}