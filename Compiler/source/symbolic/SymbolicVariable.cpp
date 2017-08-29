#include <limits>
#include <algorithm>
#include <math.h>

#include "SymbolicVariables.h"

using namespace std;

//SymbolicVariable

SymbolicVariable::SymbolicVariable(std::string name, VariableType t, Reporter &r, bool initialised):
        varN(name), type(t), reporter(r), defined(initialised) {}

const string SymbolicVariable::getName() const
{
    return varN;
}

void SymbolicVariable::setName(const std::string newName)
{
    varN = newName;
}

void SymbolicVariable::reportError(Reporter::AlertType type, string err)
{
    reporter.error(type, err);
    feasable = false;
}

bool SymbolicVariable::isDetermined()
{
    return isConst;
}

const VariableType SymbolicVariable::getType() const
{
    return type;
}

bool SymbolicVariable::isDefined() const
{
    return defined;
}

void SymbolicVariable::define()
{
    defined = true;
}

//SymbolicVariableTemplate
template <typename T>
SymbolicVariableTemplate<T>::SymbolicVariableTemplate(string name, const T lower, const T upper,
                                      Reporter& r, VariableType t, bool init): SymbolicVariable(name, t, r, init)
{
    lowerBound = lower;
    upperBound = upper;
}

template <typename T>
const T SymbolicVariableTemplate<T>::getUpperBound() const
{
    return upperBound;
}

template <typename T>
bool SymbolicVariableTemplate<T>::isFeasable()
{
    if (upperBound > lowerBound) feasable = false;
    return feasable;
}

template <typename T>
const T SymbolicVariableTemplate<T>::getLowerBound() const
{
    return lowerBound;
}

template <typename T>
const T SymbolicVariableTemplate<T>::getConstValue()
{
    if (!isDetermined()) throw "Not constant";
    return lowerBound; //could be upper
}

template <typename T>
bool SymbolicVariableTemplate<T>::isDetermined()
{
    if (lowerBound == upperBound) isConst = true;
    return isConst;
}

template <typename T>
SymbolicVariable::MeetEnum SymbolicVariableTemplate<T>::canMeet(Relations::Relop rel, T rhs) const //todo refactor and take const stuff up top
{
    switch(rel)
    {
        case Relations::EQ:
            if (isConst && getLowerBound() == rhs) return MUST;
            else if (rhs >= getLowerBound() && rhs <= getLowerBound()) return MAY;
            else return CANT;
        case Relations::NE:
            if (isConst) return (getLowerBound() != rhs) ? MUST : CANT;
            else return MAY;
        case Relations::LE:
            if (getUpperBound() <= rhs) return MUST;
            else if (getLowerBound() > rhs) return CANT;
            return MAY;
        case Relations::LT:
            if (getUpperBound() < rhs) return MUST;
            else if (getLowerBound() >= rhs) return CANT;
            return MAY;
        case Relations::GE:
            if (getLowerBound() >= rhs) return MUST;
            else if (getUpperBound() < rhs) return CANT;
            return MAY;
        case Relations::GT:
            if (getLowerBound() > rhs) return MUST;
            else if (getUpperBound() <= rhs) return CANT;
            return MAY;
    }
}

template class SymbolicVariableTemplate<double>;
template class SymbolicVariableTemplate<std::string>;