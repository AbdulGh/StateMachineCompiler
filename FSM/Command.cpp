#include <iostream>

#include "Command.h"
#include "Variable.h"

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

//todo deal with strings
/*JumpOnComparisonCommand*/
JumpOnComparisonCommand::JumpOnComparisonCommand(std::shared_ptr<Variable> varPtr, int constInt, int jstate,
                                                           JumpOnComparisonCommand::ComparisonOp type):
    effect(JUMP),
    compareTo(constInt),
    nextState(jstate),
    cop(type),
    ctype(INT)

{
    if (varPtr->getType() != Variable::Type::INT && varPtr->getType() != Variable::Type::DOUBLE) throw "Cannot compare this var.";
    var = varPtr;
}

JumpOnComparisonCommand::JumpOnComparisonCommand(std::shared_ptr<Variable> varPtr, double constDouble, int jstate,
                                                 JumpOnComparisonCommand::ComparisonOp type):
        effect(JUMP),
        compareTo(constDouble),
        nextState(jstate),
        cop(type),
        ctype(DOUBLE)

{
    if (varPtr->getType() != Variable::Type::INT && varPtr->getType() != Variable::Type::DOUBLE) throw "Cannot compare this var.";
    var = varPtr;
}

JumpOnComparisonCommand::JumpOnComparisonCommand(std::shared_ptr<Variable> varPtr1, std::shared_ptr<Variable> varPtr2, int jstate,
                                                 JumpOnComparisonCommand::ComparisonOp type):
        effect(JUMP),
        compareTo(varPtr2),
        nextState(jstate),
        cop(type),
        ctype(VAR)

{
    if (varPtr1->getType() != Variable::Type::INT && varPtr1->getType() != Variable::Type::DOUBLE) throw "Cannot compare this var.";
    var = varPtr1;
    if (varPtr2->getType() != Variable::Type::INT && varPtr2->getType() != Variable::Type::DOUBLE) throw "Cannot compare this var.";
}

void JumpOnComparisonCommand::execute()
{
    int data = (int)var->getData();
    switch (ctype)
    {
        case INT: case DOUBLE:
        {
            double compareTo = (double) compareTo;

            switch (cop)
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
            break;
        }

        case VAR:
        {
            shared_ptr<Variable> vptr = (shared_ptr<Variable>)compareTo;
            double compareTo = (double) vptr->getData();

            switch (cop)
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

            break;
        }
    }


}