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
const T SymbolicVariableTemplate<T>::canMeet(Relations::Relop rel, T rhs) const
{
    switch(rel)
    {
        case Relations::EQ:
            return rhs >= getLowerBound() && rhs <= getLowerBound();
        case Relations::NE:
            return !isConst || getUpperBound() != rhs;
        case Relations::LE:
            return getLowerBound() <= rhs;
        case Relations::LT:
            return getLowerBound() < rhs;
        case Relations::GE:
            return getUpperBound() >= rhs;
        case Relations::GT:
            return getUpperBound() > rhs;
    }
}

template class SymbolicVariableTemplate<double>;
template class SymbolicVariableTemplate<std::string>;