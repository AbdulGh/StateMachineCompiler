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
template <typename T>
std::string PrintCommand<T>::unescape(const string& in)
{
    string ret;
    string::const_iterator it = in.begin();
    while (it != in.end())
    {
        char c = *it++;
        if (c == '\\' && it != in.end())
        {
            if (++it != in.end() && *it == '\\'
                    && ++it != in.end() && *it != 'n') ret += "\n";
            else ret += "\\";
        }
        else ret += c;
    }

    return ret;
}

template<>
PrintCommand<string>::PrintCommand(string s)
{
    this->toPrint = move(s);//unescape(s);
}

template <typename T>
PrintCommand<T>::PrintCommand(T toPrint)
{
    this->toPrint = toPrint;
}

template<>
void PrintCommand<Variable*>::execute()
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

template <typename T>
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

/*ReturnCommand - jumps to the number on top of the stack if it is not empty*/
ReturnCommand::ReturnCommand(FSM &stackOwner):
        popFrom(&stackOwner.sharedStack) {}

void ReturnCommand::execute()
{
    if (popFrom->empty()) setChangeState(false);
    else
    {
        setState((int)popFrom->top().contents);
        popFrom->pop();
        setChangeState(true);
    }
}

/*InputVarCommand*/
InputVarCommand::InputVarCommand(Variable* varPtr):
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
template <typename T>
PushCommand<T>::PushCommand(T in, FSM &stackOwner):
    var(in),
    pushTo(&stackOwner.sharedStack)
{}

template<>
void PushCommand<Variable*>::execute()
{
    pushTo->push(var->getTaggedDataUnion());
}

template<class T>
void PushCommand<T>::execute()
{
    pushTo->push(Variable::TaggedDataUnion(var));
}

/*PopCommand*/
PopCommand::PopCommand(Variable* varPtr, FSM &stackOwner):
        var(move(varPtr)),
        popFrom(&stackOwner.sharedStack)
{}

void PopCommand::execute()
{
    if (popFrom->empty()) throw "tried to pop empty stack";
    if (var != nullptr) var->setData(popFrom->top());
    popFrom->pop();
}

/*AssignVarCommand*/
template <typename T>
AssignVarCommand<T>::AssignVarCommand(Variable* varPtr, T value):
        var(varPtr),
        val(value) {}

template <typename T>
void AssignVarCommand<T>::execute()
{
    var->setData(val);
}

/*EvaluateExprCommand*/
template <typename T>
EvaluateExprCommand<T>::EvaluateExprCommand(Variable* varPtr, Variable* LHSVar, T b, ExpressionType t):
    var(varPtr),
    term1(LHSVar),
    term2(b),
    type(t)
{
    if (varPtr->getType() != LHSVar->getType()) throw runtime_error("Incompatible types in evaluation");
}

template <typename T>
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
void EvaluateExprCommand<Variable*>::execute()
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
JumpOnComparisonCommand<T>::JumpOnComparisonCommand(Variable* varPtr, T compare, int jstate, ComparisonOp type):
        compareTo(compare),
        cop(type),
        var(varPtr)
        {setState(jstate);}

template <typename T>
JumpOnComparisonCommand<T>::JumpOnComparisonCommand(Variable* varPtr, T compare, FSM& stackOwner, ComparisonOp type):
        compareTo(compare),
        cop(type),
        var(varPtr),
        popFrom(&stackOwner.sharedStack){}

template <>
void JumpOnComparisonCommand<Variable*>::execute()
{
    if (var->getType() == STRING) setChangeState(evaluateComparisonOp<string>
                                                         (*(var->getData().str), cop, *(compareTo->getData().str)));
    else setChangeState(evaluateComparisonOp<double>(var->getData().d, cop, compareTo->getData().d));

    if (changesState() && getNextState() == -1)
    {
        setState((int)popFrom->top().contents);
        popFrom->pop();
    }
}


template <typename T>
void JumpOnComparisonCommand<T>::execute()
{
    T data = (T)var->getData();

    setChangeState(evaluateComparisonOp<T>(data, cop, compareTo));

    if (changesState() && getNextState() == -1)
    {
        setState((int)popFrom->top().contents);
        popFrom->pop();
    }
}

template class JumpOnComparisonCommand<double>;
template class JumpOnComparisonCommand<string>;
template class JumpOnComparisonCommand<Variable*>;
template class AssignVarCommand<double>;
template class AssignVarCommand<string>;
template class AssignVarCommand<Variable*>;
template class EvaluateExprCommand<Variable*>;
template class EvaluateExprCommand<double>;
template class PrintCommand<string>;
template class PrintCommand<Variable*>;
template class PushCommand<double>;
template class PushCommand<string>;
template class PushCommand<Variable*>;
