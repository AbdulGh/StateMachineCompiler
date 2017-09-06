#include <limits>
#include <algorithm>
#include <math.h>
#include <string>

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

bool SymbolicVariable::isFeasable() const
{
    return feasable;
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
const T& SymbolicVariableTemplate<T>::getUpperBound() const
{
    return upperBound;
}

template<typename T>
bool SymbolicVariableTemplate<T>::isDisjointFrom(shared_ptr<SymbolicVariableTemplate<T>> other)
{
    if (!isFeasable() || !other->isFeasable()) return true; //empty sets disjoint from everything
    if (!isBoundedAbove() && !isBoundedBelow() || !other->isBoundedAbove() && !other->isBoundedBelow()) return false;
    else if (isBoundedAbove() && other->isBoundedBelow() && upperBound < other->lowerBound) return true;
    else if (isBoundedBelow() && other->isBoundedAbove() && lowerBound > other->upperBound) return true;
    return false;
}

template<>
bool SymbolicVariableTemplate<string>::meetsConstComparison(Relations::Relop r, const std::string& rhs)
{
    return Relations::evaluateRelop<string>(getConstValue(), r, rhs);
}

template<>
bool SymbolicVariableTemplate<double>::meetsConstComparison(Relations::Relop r, const std::string& rhs)
{
    return Relations::evaluateRelop<double>(getConstValue(), r, stod(rhs));
}

template <typename T>
bool SymbolicVariableTemplate<T>::isFeasable()
{
    if (upperBound < lowerBound)
    {
        feasable = false;
    }
    return feasable;
}

template <typename T>
void SymbolicVariableTemplate<T>::setConstValue(const T &cv)
{
    upperBound = lowerBound = cv;
    isConst = true;
}

template <typename T>
const T& SymbolicVariableTemplate<T>::getLowerBound() const
{
    return lowerBound;
}

template <>
const string SymbolicVariableTemplate<string>::getConstString()
{
    return getConstValue();
}

template <typename T>
const string SymbolicVariableTemplate<T>::getConstString()
{
    return to_string(getConstValue());
}

template <typename T>
const T& SymbolicVariableTemplate<T>::getConstValue()
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

template class SymbolicVariableTemplate<double>;
template class SymbolicVariableTemplate<std::string>;