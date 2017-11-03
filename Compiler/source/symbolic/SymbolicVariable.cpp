#include <limits>
#include <algorithm>
#include <math.h>
#include <string>

#include "SymbolicVariables.h"

using namespace std;

//SymbolicVariable

SymbolicVariable::SymbolicVariable(std::string name, VariableType t, Reporter &r, bool initialised):
        varN(move(name)), type(t), reporter(r), defined(initialised) {}

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

//todo 'bubble' relationships closer
bool SymbolicVariable::guaranteedLT(const string& searchName, const string& initName) const
{
    if (getName() == initName || getName() == searchName)  return false;
    for (shared_ptr<SymbolicVariable>& optr : lt)
    {
        if (optr->getName() == searchName || optr->guaranteedLE(searchName, initName)) return true;
    }
    for (shared_ptr<SymbolicVariable>& optr : le) if (optr->guaranteedLT(searchName, initName)) return true;
    for (shared_ptr<SymbolicVariable>& optr : eq) if (optr->guaranteedLT(searchName, initName)) return true;
    return false;
}
bool SymbolicVariable::guaranteedLE(const string& searchName, const string& initName) const
{
    if (getName() == initName || getName() == searchName) return true;
    for (shared_ptr<SymbolicVariable>& optr : lt) if (optr->guaranteedLE(searchName, initName)) return true;
    for (shared_ptr<SymbolicVariable>& optr : le) if (optr->guaranteedLE(searchName, initName)) return true;
    for (shared_ptr<SymbolicVariable>& optr : eq) if (optr->guaranteedLE(searchName, initName)) return true;
    return false;
}
bool SymbolicVariable::guaranteedGT(const string& searchName, const string& initName) const
{
    if (getName() == initName || getName() == searchName)  return false;
    for (shared_ptr<SymbolicVariable>& optr : gt)
    {
        if (optr->getName() == searchName || optr->guaranteedGE(searchName, initName)) return true;
    }
    for (shared_ptr<SymbolicVariable>& optr : ge) if (optr->guaranteedGT(searchName, initName)) return true;
    for (shared_ptr<SymbolicVariable>& optr : eq) if (optr->guaranteedGT(searchName, initName)) return true;
    return false;
}
bool SymbolicVariable::guaranteedGE(const string& searchName, const string& initName) const
{
    if (getName() == initName || getName() == searchName) return true;
    for (shared_ptr<SymbolicVariable>& optr : gt) if (optr->guaranteedGE(searchName, initName)) return true;
    for (shared_ptr<SymbolicVariable>& optr : ge) if (optr->guaranteedGE(searchName, initName)) return true;
    for (shared_ptr<SymbolicVariable>& optr : eq) if (optr->guaranteedGE(searchName, initName)) return true;
    return false;
}
bool SymbolicVariable::guaranteedEQ(const string& searchName, const string& initName) const
{
    if (getName() == initName || getName() == searchName) return true;
    for (shared_ptr<SymbolicVariable>& optr : eq) if (optr->guaranteedEQ(searchName, initName)) return true;
    return false;
}

bool SymbolicVariable::guaranteedNEQ(const string& searchName, const string& initName) const
{
    if (getName() == initName || getName() == searchName) return false;
    for (shared_ptr<SymbolicVariable>& optr : neq) if (optr->guaranteedEQ(searchName, initName)) return true;
    for (shared_ptr<SymbolicVariable>& optr : gt) if (optr->guaranteedGE(searchName, initName)) return true;
    for (shared_ptr<SymbolicVariable>& optr : ge) if (optr->guaranteedGT(searchName, initName)) return true;
    for (shared_ptr<SymbolicVariable>& optr : lt) if (optr->guaranteedLE(searchName, initName)) return true;
    for (shared_ptr<SymbolicVariable>& optr : le) if (optr->guaranteedLE(searchName, initName)) return true;
    return false;
}

//SymbolicVariableTemplate
template <typename T>
SymbolicVariableTemplate<T>::SymbolicVariableTemplate(string name, const T lower, const T upper,
                                      Reporter& r, VariableType t, bool init): SymbolicVariable(name, t, r, init)
{
    lowerBound = lower;
    upperBound = upper;
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
    return Relations::evaluateRelop<string>(getTConstValue(), r, rhs);
}

template<>
bool SymbolicVariableTemplate<double>::meetsConstComparison(Relations::Relop r, const string& rhs)
{
    return Relations::evaluateRelop<double>(getTConstValue(), r, stod(rhs));
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
void SymbolicVariableTemplate<T>::setTConstValue(const T &cv)
{
    define();
    upperBound = lowerBound = cv;
    isConst = true;
}

template <typename T>
const T& SymbolicVariableTemplate<T>::getTLowerBound() const
{
    return lowerBound;
}

template <typename T>
const T& SymbolicVariableTemplate<T>::getTUpperBound() const
{
    return upperBound;
}

template <>
string SymbolicVariableTemplate<string>::getLowerBound() const
{
    return lowerBound;
}
template <>
string SymbolicVariableTemplate<string>::getUpperBound() const
{
    return upperBound;
}

template <typename T>
string SymbolicVariableTemplate<T>::getLowerBound() const
{
    return to_string(lowerBound);
}
template <typename T>
string SymbolicVariableTemplate<T>::getUpperBound() const
{
    return to_string(upperBound);
}

template <>
const string SymbolicVariableTemplate<string>::getConstString()
{
    return getTConstValue();
}

template <typename T>
const string SymbolicVariableTemplate<T>::getConstString()
{
    return to_string(getTConstValue());
}

template <typename T>
const T& SymbolicVariableTemplate<T>::getTConstValue()
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