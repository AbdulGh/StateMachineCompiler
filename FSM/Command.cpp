#include <iostream>

#include "Command.h"

//todo refactor to use templates

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
        case Type::STRING:
            cout << (char*) var->getData();
            break;
        case Type::INT:
            cout << (int) var->getData();
            break;
        case Type::DOUBLE:
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
        case Type::STRING:
        {
            string str;
            cin >> str;
            var->setData(str);
            break;
        }
        case Type::INT:
        {
            int i;
            cin >> i;
            var->setData(i);
            break;
        }
        case Type::DOUBLE:
        {
            int d;
            cin >> d;
            var->setData(d);
            break;
        }
    }
}

template <class T>
AssignVarCommand<T>::AssignVarCommand(std::shared_ptr<Variable> varPtr, T value):
        var(varPtr),
        effect(SETVAR),
        val(value) {}

template <class T>
void AssignVarCommand<T>::execute()
{
    var->setData(val);
}

//todo deal with strings

//todo carry on refactoring this w/ templates(HERE)
/*JumpOnComparisonCommand*/
template <typename T>
JumpOnComparisonCommand<T>::JumpOnComparisonCommand(std::shared_ptr<Variable> varPtr, T compare, int jstate, ComparisonOp type):
        effect(JUMP),
        compareTo(compare),
        nextState(jstate),
        cop(type)

{
    if (varPtr->getType() != Type::INT && varPtr->getType() != Type::DOUBLE) throw "Cannot compare this var.";
    var = varPtr;
}

template <typename T>
void JumpOnComparisonCommand<T>::execute()
{
    int data = (int)var->getData();
    double RHS = (double) compareTo;

    switch (cop)
    {
        case GT:
            changeState = data > RHS;
            break;
        case GE:
            changeState = data >= RHS;
            break;
        case LT:
            changeState = data < RHS;
            break;
        case LE:
            changeState = data <= RHS;
            break;
        case EQ:
            changeState = data == RHS;
            break;
        case NEQ:
            changeState = data != RHS;
            break;
    }
}

template<>
void JumpOnComparisonCommand<std::shared_ptr<Variable>>::execute()
{
    double data = (double) var->getData();
    double RHS = compareTo->getData();

    switch (cop)
    {
        case GT:
            changeState = data > RHS;
            break;
        case GE:
            changeState = data >= RHS;
            break;
        case LT:
            changeState = data < RHS;
            break;
        case LE:
            changeState = data <= RHS;
            break;
        case EQ:
            changeState = data == RHS;
            break;
        case NEQ:
            changeState = data != RHS;
            break;
    }
}

template class JumpOnComparisonCommand<double>;
template class JumpOnComparisonCommand<std::shared_ptr<Variable>>;
template class AssignVarCommand<double>;
template class AssignVarCommand<std::shared_ptr<Variable>>;