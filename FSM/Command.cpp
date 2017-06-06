#include <iostream>
#include <math.h>

#include "Command.h"
#include "FSM.h"

using namespace std;

int AbstractCommand::getNextState() const
{
    return nextState;
}

bool AbstractCommand::changesState() const
{
    return changeState;
}

void AbstractCommand::setState(int i)
{
    nextState = i;
}

void AbstractCommand::setChangeState(bool b)
{
    changeState = b;
}

/*PrintCommand*/
template <class T>
PrintCommand<T>::PrintCommand(T toPrint)
{
    this->toPrint = toPrint;
}

template<>
void PrintCommand<std::shared_ptr<Variable>>::execute()
{
    switch(toPrint->getType())
    {
        case Type::STRING:
        {
            string *ref = toPrint->getData();
            cout << *ref;
            break;
        }
        case Type::DOUBLE:
        {
            cout << (double) toPrint->getData();
            break;
        }
    }
}

template <class T>
void PrintCommand<T>::execute()
{
    cout << toPrint;
}

/*JumpCommand*/
JumpCommand::JumpCommand(int state)
{
    setState(state);
    setChangeState(true);
}

void JumpCommand::execute(){}

/*InputVarCommand*/
InputVarCommand::InputVarCommand(std::shared_ptr<Variable> varPtr):
        var(varPtr) {}

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
        case Type::DOUBLE:
        {
            int d;
            cin >> d;
            var->setData(d);
            break;
        }
        default:
            throw runtime_error("Strange type");
    }
}

/*PushCommand*/
template <class T>
PushCommand<T>::PushCommand(T in, FSM &stackOwner):
    var(in),
    pushTo(&stackOwner.sharedStack)
{}

template<>
void PushCommand<std::shared_ptr<Variable>>::execute()
{
    pushTo->push(var->getTaggedDataUnion());
}

template<class T>
void PushCommand<T>::execute()
{
    pushTo->push(Variable::TaggedDataUnion(var));
}

/*PopCommand*/
PopCommand::PopCommand(std::shared_ptr<Variable> varPtr, FSM &stackOwner):
        var(varPtr),
        popFrom(&stackOwner.sharedStack)
{}

void PopCommand::execute()
{
    var->setData(popFrom->top());
    popFrom->pop();
}

/*AssignVarCommand*/
template <class T>
AssignVarCommand<T>::AssignVarCommand(std::shared_ptr<Variable> varPtr, T value):
        var(varPtr),
        val(value) {}

template <class T>
void AssignVarCommand<T>::execute()
{
    var->setData(val);
}

/*EvaluateExprCommand*/
template <class T>
EvaluateExprCommand<T>::EvaluateExprCommand(std::shared_ptr<Variable> varPtr, std::shared_ptr<Variable> LHSVar, T b, ExpressionType t):
    var(varPtr),
    term1(LHSVar),
    term2(b),
    type(t)
{
    if (varPtr->getType() != LHSVar->getType()) throw runtime_error("Incompatible types in evaluation");
}

template <class T>
void EvaluateExprCommand<T>::evaluate(double one, double two)
{
    switch(type)
    {
        case MUL:
            var->setData(one * two);
            break;
        case DIV:
            var->setData(one / two);
            break;
        case PLUS:
            var->setData(one + two);
            break;
        case MINUS:
            var->setData(one - two);
            break;
        case MOD:
            var->setData(fmod(one, two));
            break;
        case POW:
            var->setData(pow(one, two));
            break;
        case AND:
            var->setData((int)one & (int)two);
            break;
        case OR:
            var->setData((int)one | (int)two);
            break;
        default:
            throw runtime_error("Weird comparison");
    }
}

/*EvaluateExprCommand*/
template <>
void EvaluateExprCommand<std::shared_ptr<Variable>>::execute()
{
    double d1 = (double) term1->getData();
    double d2 = (double) term2->getData();
    evaluate(d1, d2);
}

template <>
void EvaluateExprCommand<double>::execute()
{
    double d1 = (double) term1->getData();
    evaluate(d1, term2);
}

/*JumpOnComparisonCommand*/
template <typename T>
JumpOnComparisonCommand<T>::JumpOnComparisonCommand(std::shared_ptr<Variable> varPtr, T compare, int jstate, ComparisonOp type):
        compareTo(compare),
        cop(type),
        var(varPtr) {setState(jstate);}

template <class T>
void JumpOnComparisonCommand<T>::evaluate(double RHS)
{
    double data = var->getData();

    switch (cop)
    {
        case GT:
            setChangeState(data > RHS);
            break;
        case GE:
            setChangeState(data >= RHS);
            break;
        case LT:
            setChangeState(data < RHS);
            break;
        case LE:
            setChangeState(data <= RHS);
            break;
        case EQ:
            setChangeState(data == RHS);
            break;
        case NEQ:
            setChangeState(data != RHS);
            break;
    }
}

template <class T>
void JumpOnComparisonCommand<T>::evaluate(string RHS)
{
    string* dataP = var->getData();
    string data = *dataP;

    switch (cop)
    {
        case GT:
            setChangeState(data > RHS);
            break;
        case GE:
            setChangeState(data >= RHS);
            break;
        case LT:
            setChangeState(data < RHS);
            break;
        case LE:
            setChangeState(data <= RHS);
            break;
        case EQ:
            setChangeState(data == RHS);
            break;
        case NEQ:
            setChangeState(data != RHS);
            break;
    }
}

template<>
void JumpOnComparisonCommand<std::shared_ptr<Variable>>::execute()
{
    if (compareTo->getType() == STRING)
    {
        string* p = compareTo->getData();
        evaluate(*p);
    }
    else if (compareTo->getType() == DOUBLE) evaluate((double)compareTo->getData());
    else throw runtime_error("Weird data type");
}

template <>
void JumpOnComparisonCommand<double>::execute()
{
    evaluate(compareTo);
}

template <>
void JumpOnComparisonCommand<string>::execute()
{
    evaluate(compareTo);
}

template class JumpOnComparisonCommand<double>;
template class JumpOnComparisonCommand<string>;
template class JumpOnComparisonCommand<std::shared_ptr<Variable>>;
template class AssignVarCommand<double>;
template class AssignVarCommand<string>;
template class AssignVarCommand<std::shared_ptr<Variable>>;
template class EvaluateExprCommand<std::shared_ptr<Variable>>;
template class EvaluateExprCommand<double>;
template class PrintCommand<string>;
template class PrintCommand<std::shared_ptr<Variable>>;
template class PushCommand<double>;
template class PushCommand<string>;
template class PushCommand<std::shared_ptr<Variable>>;