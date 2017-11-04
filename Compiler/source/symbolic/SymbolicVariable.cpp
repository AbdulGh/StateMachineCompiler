#include <limits>
#include <algorithm>
#include <cmath>
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

void SymbolicVariable::addGT(const std::shared_ptr<SymbolicVariable>& other)
{
    if (find_if(gt.begin(), gt.end(),
                [&, other] (const shared_ptr<SymbolicVariable>& t)
                {return t->getName() == other->getName();}) != gt.end()) return;
    gt.push_back(other);
    other->lt.push_back(shared_from_this());
    if (other->isBoundedBelow()) clipLowerBound(other->getLowerBound(), false);
    if (isBoundedAbove()) other->clipUpperBound(getUpperBound());
}

void SymbolicVariable::addGE(const std::shared_ptr<SymbolicVariable>& other)
{
    if (find_if(ge.begin(), ge.end(),
                [&, other] (const shared_ptr<SymbolicVariable> t)
                {return t->getName() == other->getName();}) != ge.end()) return;
    ge.push_back(other);
    other->le.push_back(shared_from_this());

    if (other->isBoundedBelow()) clipLowerBound(other->getLowerBound());
    if (isBoundedAbove()) other->clipUpperBound(getUpperBound());
}

void SymbolicVariable::addLT(const std::shared_ptr<SymbolicVariable>& other)
{
    other->addGT(shared_from_this());
}

void SymbolicVariable::addLE(const std::shared_ptr<SymbolicVariable>& other)
{
    other->addGE(shared_from_this());
}

void SymbolicVariable::addEQ(const shared_ptr<SymbolicVariable>& other)
{
    if (find_if(eq.begin(), eq.end(),
        [&, other] (const shared_ptr<SymbolicVariable> t)
        {return t->getName() == other->getName();}) != eq.end()) return;
    eq.push_back(other);
    other->eq.push_back(shared_from_this());
}

void SymbolicVariable::addNEQ(const std::shared_ptr<SymbolicVariable>& other)
{
    if (find_if(neq.begin(), neq.end(),
                [&, other] (const shared_ptr<SymbolicVariable> t)
                {return t->getName() == other->getName();}) != neq.end()) return;
    neq.push_back(other);
    other->neq.push_back(shared_from_this());
}

//todo 'bubble' relationships closer
bool SymbolicVariable::guaranteedLT(const shared_ptr<SymbolicVariable>& searchFor, const string& initName)
{
    if (getName() == initName || getName() == searchFor->getName())  return false;
    for (const shared_ptr<SymbolicVariable>& optr : lt)
    {
        if (optr->getName() == searchFor->getName() || optr->guaranteedLE(searchFor, initName))
        {
            addLT(searchFor);
            return true;
        }
    }
    for (const shared_ptr<SymbolicVariable>& optr : le) if (optr->guaranteedLT(searchFor, initName))
    {
        addLT(searchFor);
        return true;
    }
    for (const shared_ptr<SymbolicVariable>& optr : eq) if (optr->guaranteedLT(searchFor, initName))
    {
        addLT(searchFor);
        return true;
    }
    return false;
}
bool SymbolicVariable::guaranteedLE(const shared_ptr<SymbolicVariable>& searchFor, const string& initName)
{
    if (getName() == initName || getName() == searchFor->getName()) return true;
    for (const shared_ptr<SymbolicVariable>& optr : lt) if (optr->guaranteedLE(searchFor, initName))
    {
        addLE(searchFor);
        return true;
    }
    for (const shared_ptr<SymbolicVariable>& optr : le) if (optr->guaranteedLE(searchFor, initName))
    {
        addLE(searchFor);
        return true;
    }
    for (const shared_ptr<SymbolicVariable>& optr : eq) if (optr->guaranteedLE(searchFor, initName))
    {
        addLE(searchFor);
        return true;
    }
    return false;
}
bool SymbolicVariable::guaranteedGT(const shared_ptr<SymbolicVariable>& searchFor, const string& initName)
{
    if (getName() == initName || getName() == searchFor->getName())  return false;
    for (const shared_ptr<SymbolicVariable>& optr : gt)
    {
        if (optr->getName() == searchFor->getName() || optr->guaranteedGE(searchFor, initName))
        {
            addGT(searchFor);
            return true;
        }
    }
    for (const shared_ptr<SymbolicVariable>& optr : ge) if (optr->guaranteedGT(searchFor, initName))
    {
        addGT(searchFor);
        return true;
    }
    for (const shared_ptr<SymbolicVariable>& optr : eq) if (optr->guaranteedGT(searchFor, initName))
    {
        addGT(searchFor);
        return true;
    }
    return false;
}
bool SymbolicVariable::guaranteedGE(const shared_ptr<SymbolicVariable>& searchFor, const string& initName)
{
    if (getName() == initName || getName() == searchFor->getName()) return true;
    for (const shared_ptr<SymbolicVariable>& optr : gt) if (optr->guaranteedGE(searchFor, initName))
    {
        addGE(searchFor);
        return true;
    }
    for (const shared_ptr<SymbolicVariable>& optr : ge) if (optr->guaranteedGE(searchFor, initName))
    {
        addGE(searchFor);
        return true;
    }
    for (const shared_ptr<SymbolicVariable>& optr : eq) if (optr->guaranteedGE(searchFor, initName))
    {
        addGE(searchFor);
        return true;
    }
    return false;
}
bool SymbolicVariable::guaranteedEQ(const shared_ptr<SymbolicVariable>& searchFor, const string& initName)
{
    if (getName() == initName || getName() == searchFor->getName()) return true;
    for (const shared_ptr<SymbolicVariable>& optr : eq) if (optr->guaranteedEQ(searchFor, initName))
    {
        addEQ(searchFor);
        return true;
    }
    return false;
}

bool SymbolicVariable::guaranteedNEQ(const shared_ptr<SymbolicVariable>& searchFor, const string& initName)
{
    if (getName() == initName || getName() == searchFor->getName()) return false;
    for (const shared_ptr<SymbolicVariable>& optr : neq) if (optr->guaranteedEQ(searchFor, initName))
        {addNEQ(searchFor); return true;}
    for (const shared_ptr<SymbolicVariable>& optr : gt) if (optr->guaranteedGE(searchFor, initName))
        {addNEQ(searchFor); return true;}
    for (const shared_ptr<SymbolicVariable>& optr : ge) if (optr->guaranteedGT(searchFor, initName))
        {addNEQ(searchFor); return true;}
    for (const shared_ptr<SymbolicVariable>& optr : lt) if (optr->guaranteedLE(searchFor, initName))
        {addNEQ(searchFor); return true;}
    for (const shared_ptr<SymbolicVariable>& optr : le) if (optr->guaranteedLE(searchFor, initName))
        {addNEQ(searchFor); return true;}
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
    if (upperBound < lowerBound) feasable = false;
    return feasable;
}

template <typename T>
void SymbolicVariableTemplate<T>::addEQ(const std::shared_ptr<SymbolicVariable>& other)
{
    SymbolicVariable::addEQ(other);
    shared_ptr<SymbolicVariableTemplate<T>> otherCast = static_pointer_cast<SymbolicVariableTemplate<T>>(other);
    if (otherCast->isBoundedBelow())
    {
        if (isBoundedBelow())
        {
            if (getTLowerBound() < otherCast->getTLowerBound()) setTLowerBound(otherCast->getTLowerBound(), true);
            else otherCast->setTLowerBound(getTLowerBound(), true);
        }
        else setTLowerBound(otherCast->getTLowerBound(), true);
    }
    else if (isBoundedBelow()) otherCast->setTLowerBound(getTLowerBound(), true);

    if (otherCast->isBoundedAbove())
    {
        if (isBoundedAbove())
        {
            if (getTUpperBound() < otherCast->getTUpperBound()) otherCast->setTUpperBound(getTUpperBound(), true);
            else setTUpperBound(otherCast->getTUpperBound(), true);
        }
        else setTUpperBound(otherCast->getTUpperBound(), true);
    }
    else if (isBoundedAbove()) otherCast->setTUpperBound(getTUpperBound(), true);
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