#include <limits>
#include <algorithm>
#include <cmath>
#include <string>

#include "SymbolicVariables.h"

using namespace std;

//SymbolicVariable

SymbolicVariable::SymbolicVariable(string name, VariableType t, Reporter* r, bool initialised, bool f):
        varN(move(name)), type(t), reporter(r), defined(initialised), feasable(f), incrementable(type == DOUBLE) {}

SymbolicVariable::~SymbolicVariable()
{
    clearEQ();
    clearGreater();
    clearLess();
}

const string& SymbolicVariable::getName() const
{
    return varN;
}

void SymbolicVariable::setName(const string& newName)
{
    varN = newName;
}

void SymbolicVariable::reportError(Reporter::AlertType type, string err)
{
    reporter->error(type, err);
    feasable = false;
}

const VariableType SymbolicVariable::getType() const
{
    return type;
}

bool SymbolicVariable::isDefined() const
{
    return defined;
}

bool SymbolicVariable::wasUserAffected() const
{
    return userAffected;
}

bool SymbolicVariable::isIncrementable() const
{
    return incrementable;
}

void SymbolicVariable::define()
{
    defined = true;
}

bool SymbolicVariable::isFeasable()
{
    return feasable;
}

bool SymbolicVariable::addGT(SymbolicVariable* other)
{
    if (find_if(gt.begin(), gt.end(),
                [&, other] (SymbolicVariable* t)
                {return t->getName() == other->getName();}) != gt.end()) return feasable && other->isFeasable();
    gt.push_back(other);
    other->lt.push_back(this);
    if (other->isBoundedBelow()) clipLowerBound(other->getLowerBound(), false);
    if (isBoundedAbove()) other->clipUpperBound(getUpperBound());
    return feasable && other->isFeasable();
}

bool SymbolicVariable::addGE(SymbolicVariable* other)
{
    if (find_if(ge.begin(), ge.end(),
                [&, other] (SymbolicVariable* t)
                {return t->getName() == other->getName();}) != ge.end()) return feasable && other->isFeasable();
    ge.push_back(other);
    other->le.push_back(this);

    if (other->isBoundedBelow()) clipLowerBound(other->getLowerBound());
    if (isBoundedAbove()) other->clipUpperBound(getUpperBound());
    return feasable && other->isFeasable();
}

bool SymbolicVariable::addLT(SymbolicVariable* other)
{
    other->addGT(this);
    return feasable && other->isFeasable();
}

bool SymbolicVariable::addLE(SymbolicVariable* other)
{
    other->addGE(this);
    return feasable && other->isFeasable();
}

bool SymbolicVariable::addEQ(SymbolicVariable* other)
{
    if (find_if(eq.begin(), eq.end(),
        [&, other] (SymbolicVariable* t)
        {return t->getName() == other->getName();}) != eq.end()) return feasable && other->isFeasable();
    eq.push_back(other);
    other->eq.push_back(this);
    return feasable && other->isFeasable();
}

bool SymbolicVariable::addNEQ(SymbolicVariable* other)
{
    if (find_if(neq.begin(), neq.end(),
                [&, other] (SymbolicVariable* t)
                {return t->getName() == other->getName();}) != neq.end()) return feasable && other->isFeasable();
    neq.push_back(other);
    other->neq.push_back(this);
    return feasable && other->isFeasable();
}

bool SymbolicVariable::guaranteedLT(SymbolicVariable* searchFor, const string& initName)
{
    if (getName() == initName || getName() == searchFor->getName())  return false;
    for (SymbolicVariable* optr : lt)
    {
        if (optr->getName() == searchFor->getName() || optr->guaranteedLE(searchFor, initName))
        {
            addLT(searchFor);
            return true;
        }
    }
    for (SymbolicVariable* optr : le) if (optr->guaranteedLT(searchFor, initName))
    {
        addLT(searchFor);
        return true;
    }
    for (SymbolicVariable* optr : eq) if (optr->guaranteedLT(searchFor, initName))
    {
        addLT(searchFor);
        return true;
    }
    return false;
}
bool SymbolicVariable::guaranteedLE(SymbolicVariable* searchFor, const string& initName)
{
    if (getName() == initName || getName() == searchFor->getName()) return true;
    for (SymbolicVariable* optr : lt) if (optr->guaranteedLE(searchFor, initName))
    {
        addLE(searchFor);
        return true;
    }
    for (SymbolicVariable* optr : le) if (optr->guaranteedLE(searchFor, initName))
    {
        addLE(searchFor);
        return true;
    }
    for (SymbolicVariable* optr : eq) if (optr->guaranteedLE(searchFor, initName))
    {
        addLE(searchFor);
        return true;
    }
    return false;
}
bool SymbolicVariable::guaranteedGT(SymbolicVariable* searchFor, const string& initName)
{
    if (getName() == initName || getName() == searchFor->getName())  return false;
    for (SymbolicVariable* optr : gt)
    {
        if (optr->getName() == searchFor->getName() || optr->guaranteedGE(searchFor, initName))
        {
            addGT(searchFor);
            return true;
        }
    }
    for (SymbolicVariable* optr : ge) if (optr->guaranteedGT(searchFor, initName))
    {
        addGT(searchFor);
        return true;
    }
    for (SymbolicVariable* optr : eq) if (optr->guaranteedGT(searchFor, initName))
    {
        addGT(searchFor);
        return true;
    }
    return false;
}
bool SymbolicVariable::guaranteedGE(SymbolicVariable* searchFor, const string& initName)
{
    if (getName() == initName || getName() == searchFor->getName()) return true;
    for (SymbolicVariable* optr : gt) if (optr->guaranteedGE(searchFor, initName))
    {
        addGE(searchFor);
        return true;
    }
    for (SymbolicVariable* optr : ge) if (optr->guaranteedGE(searchFor, initName))
    {
        addGE(searchFor);
        return true;
    }
    for (SymbolicVariable* optr : eq) if (optr->guaranteedGE(searchFor, initName))
    {
        addGE(searchFor);
        return true;
    }
    return false;
}
bool SymbolicVariable::guaranteedEQ(SymbolicVariable* searchFor, const string& initName)
{
    if (getName() == initName || getName() == searchFor->getName()) return true;
    for (SymbolicVariable* optr : eq) if (optr->guaranteedEQ(searchFor, initName))
    {
        addEQ(searchFor);
        return true;
    }
    return false;
}

bool SymbolicVariable::guaranteedNEQ(SymbolicVariable* searchFor, const string& initName)
{
    if (getName() == initName || getName() == searchFor->getName()) return false;
    for (SymbolicVariable* optr : neq) if (optr->guaranteedEQ(searchFor, initName))
        {addNEQ(searchFor); return true;}
    for (SymbolicVariable* optr : gt) if (optr->guaranteedGE(searchFor, initName))
        {addNEQ(searchFor); return true;}
    for (SymbolicVariable* optr : ge) if (optr->guaranteedGT(searchFor, initName))
        {addNEQ(searchFor); return true;}
    for (SymbolicVariable* optr : lt) if (optr->guaranteedLE(searchFor, initName))
        {addNEQ(searchFor); return true;}
    for (SymbolicVariable* optr : le) if (optr->guaranteedLE(searchFor, initName))
        {addNEQ(searchFor); return true;}
    for (SymbolicVariable* optr : eq) if (optr->guaranteedNEQ(searchFor, initName))
        {addNEQ(searchFor); return true;}
    return false;
}

void SymbolicVariable::clearEQ()
{
    auto isMe = [&, this](SymbolicVariable* poss) -> bool
    {return poss->getName() == getName();};
    for (auto& ptr : eq)
    {
        auto& vector = ptr->eq;
        vector.erase(remove_if(vector.begin(), vector.end(), isMe), vector.end());
    }
    eq.clear();
    for (auto& ptr : neq)
    {
        auto& vector = ptr->neq;
        vector.erase(remove_if(vector.begin(), vector.end(), isMe), vector.end());
    }
    neq.clear();
}

void SymbolicVariable::clearLess()
{
    auto isMe = [&, this](SymbolicVariable* poss) -> bool
    {return poss->getName() == getName();};
    for (auto& ptr : lt)
    {
        auto& vector = ptr->gt;
        vector.erase(remove_if(vector.begin(), vector.end(), isMe), vector.end());
    }
    lt.clear();
    for (auto& ptr : le)
    {
        auto& vector = ptr->ge;
        vector.erase(remove_if(vector.begin(), vector.end(), isMe), vector.end());
    }
    le.clear();
}

void SymbolicVariable::clearAll()
{
    clearEQ();
    clearEQ();
    clearLess();
}

void SymbolicVariable::userInput()
{
    userAffected = true;
    defined = true;
}

void SymbolicVariable::clearGreater()
{
    auto isMe = [&, this](SymbolicVariable* poss) -> bool
    {return poss->getName() == getName();};
    for (auto& ptr : gt)
    {
        auto& vector = ptr->lt;
        vector.erase(remove_if(vector.begin(), vector.end(), isMe), vector.end());
    }
    gt.clear();
    for (auto& ptr : ge)
    {
        auto& vector = ptr->le;
        vector.erase(remove_if(vector.begin(), vector.end(), isMe), vector.end());
    }
    ge.clear();
}

//SymbolicVariableTemplate
template <typename T>
SymbolicVariableTemplate<T>::SymbolicVariableTemplate(string name, const T lower, const T upper,
                                      Reporter* r, VariableType t, bool init):
        SymbolicVariable(name, t, r, init, lower <= upper), lowerBound(lower), upperBound(upper)
{}

template<typename T>
bool SymbolicVariableTemplate<T>::isDisjointFrom(shared_ptr<SymbolicVariableTemplate<T>> other)
{
    if (!isFeasable() || !other->isFeasable()) return true; //empty set is disjoint from everything
    if (!isBoundedAbove() && !isBoundedBelow() || !other->isBoundedAbove() && !other->isBoundedBelow()) return false;
    else if (isBoundedAbove() && other->isBoundedBelow() && upperBound < other->lowerBound) return true;
    else if (isBoundedBelow() && other->isBoundedAbove() && lowerBound > other->upperBound) return true;
    return false;
}

template<>
bool SymbolicVariableTemplate<string>::meetsConstComparison(Relations::Relop r, const string& rhs)
{
    return Relations::evaluateRelop<string>(getTConstValue(), r, rhs);
}

template <typename T>
SymbolicVariable::MeetEnum SymbolicVariableTemplate<T>::canMeet(Relations::Relop rel, SymbolicVariable *rhs) const
{
    throw "not implemented for generic vars";
    /*
    if (r->isDetermined()) return canMeet(rel, r->getConstString());
    SymbolicDouble* rhs = static_cast<SymbolicDouble*>(r);

    bool upperLessThanLower = !isBoundedAbove() && !rhs->isBoundedAbove()

    switch(rel)
    {
        case Relations::EQ:
            if ((isBoundedAbove() && rhs->isBoundedBelow() && getTUpperBound() < rhs->getTLowerBound())
                || isBoundedBelow() && rhs->isBoundedAbove() && getTLowerBound() > rhs->getTLowerBound())
            {
                return CANT;
            }
            return MAY;
        case Relations::NEQ:
            if (isBoundedAbove() && isBoundedBelow()
                    && (!rhs->isBoundedBelow() || rhs->getTLowerBound() <= getTLowerBound())
                    && (!rhs->isBoundedAbove() || rhs->getTUpperBound() >= getTUpperBound()))
            {
                return CANT;
            }
            return MAY;
        case Relations::LE:
            if (!rhs->isBoundedBelow() && !rhs->isBoundedAbove()) return MUST;
            if (!rhs->isBoundedBelow())
            {
                if (!isBoundedBelow() || (!rhs->isBoundedAbove() || rhs->getTUpperBound() > getTUpperBound()))
            }
        case Relations::LT:
            if (getTUpperBound() < rhs) return MUST;
            else if (getTLowerBound() >= rhs) return CANT;
            return MAY;
        case Relations::GE:
            if (getTLowerBound() >= rhs) return MUST;
            else if (getTUpperBound() < rhs) return CANT;
            return MAY;
        case Relations::GT:
            if (getTLowerBound() > rhs) return MUST;
            else if (getTUpperBound() <= rhs) return CANT;
            return MAY;
    }*/
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

template <>
void SymbolicVariableTemplate<string>::addNEQConst(const string& c)
{
    if (find(neqConsts.begin(), neqConsts.end(), c) != neqConsts.end()) return;
    neqConsts.push_back(c);
}

template <>
void SymbolicVariableTemplate<double>::addNEQConst(const string& d)
{
    double c = stod(d);
    if (find(neqConsts.begin(), neqConsts.end(), c) != neqConsts.end()) return;
    neqConsts.push_back(c);
}

template <typename T>
bool SymbolicVariableTemplate<T>::addEQ(SymbolicVariable* other)
{
    SymbolicVariable::addEQ(other);
    SymbolicVariableTemplate<T>* otherCast = static_cast<SymbolicVariableTemplate<T>*>(other);
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
    return isFeasable();
}

template <typename T>
void SymbolicVariableTemplate<T>::clearEQ()
{
    SymbolicVariable::clearEQ();
    neqConsts.clear();
}

template <typename T>
void SymbolicVariableTemplate<T>::setTConstValue(const T &cv)
{
    define();
    clearAll(); //todo make forgetting more intelligent
    upperBound = lowerBound = cv;
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
bool SymbolicVariableTemplate<T>::isDetermined() const
{
    return lowerBound == upperBound;
}

template class SymbolicVariableTemplate<double>;
template class SymbolicVariableTemplate<string>;