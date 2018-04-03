#include <limits>
#include <algorithm>
#include <string>

#include "SymbolicVariables.h"
#include "VarWrappers.h"

using namespace std;
using namespace SymbolicExecution;

//SymbolicVariable

SymbolicVariable::SymbolicVariable(string name, VariableType t, Reporter& r, bool def, bool f, bool inc):
        varN(move(name)), type(t), reporter(r), defined(def), feasable(f), incrementable(inc) {}

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
    reporter.error(type, err);
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

bool SymbolicVariable::getRelativeVelocity(SymbolicVariable *o, long double& slowest, long double& fastest) const
{
    return false;
}

void SymbolicVariable::define()
{
    defined = true;
}

bool SymbolicVariable::isFeasable()
{
    return feasable;
}


bool SymbolicVariable::addGT(const VarWrapper* vg, SymbolicExecutionFringe* sef, bool constructed)
{
    GottenVarPtr<SymbolicVariable> other = vg->getSymbolicVariable(sef);
    SymbolicVariable* sv = other.get();
    if (!other.constructed())
    {
        if (find_if(gt.begin(), gt.end(),
                    [&, sv] (SymbolicVariable* t)
                    {return t->getName() == sv->getName();}) != gt.end()) return isFeasable() && sv->isFeasable();
        gt.insert(sv);
    }
    if (!constructed) sv->lt.insert(this);
    if (sv->isBoundedBelow())
    {
        clipLowerBound(sv->getLowerBound(), false);
        setRepeatLowerBound(sv->getLowerBound(), false);
    }
    else removeRepeatLowerBound();
    if (isBoundedAbove())
    {
        sv->clipUpperBound(getUpperBound());
        sv->setRepeatLowerBound(getUpperBound());
    }
    else sv->removeRepeatUpperBound();
    return isFeasable() && sv->isFeasable();
}

bool SymbolicVariable::addGE(const VarWrapper* vg, SymbolicExecutionFringe* sef,  bool constructed)
{
    GottenVarPtr<SymbolicVariable> other = vg->getSymbolicVariable(sef);
    SymbolicVariable* sv = other.get();

    if (!other.constructed())
    {
        if (find_if(ge.begin(), ge.end(),
                    [&, sv] (SymbolicVariable* t)
                    {return t->getName() == sv->getName();}) != ge.end()) return isFeasable() && sv->isFeasable();
        ge.insert(sv);
    }
    if (!constructed) sv->le.insert(this);
    
    if (sv->isBoundedBelow())
    {
        clipLowerBound(sv->getLowerBound());
        setRepeatLowerBound(sv->getLowerBound());
    }
    else removeRepeatLowerBound();
    if (isBoundedAbove())
    {
        sv->clipUpperBound(getUpperBound());
        sv->setRepeatUpperBound(getUpperBound());
    }
    else sv->removeRepeatUpperBound();
    return isFeasable() && sv->isFeasable();
}

bool SymbolicVariable::addLT(const VarWrapper* vg, SymbolicExecutionFringe* sef,  bool constructed)
{
    GottenVarPtr<SymbolicVariable> other = vg->getSymbolicVariable(sef);
    SymbolicVariable* sv = other.get();
    if (!constructed && !other.constructed())
    {
        set<SymbolicVariable*> seen;
        if (guaranteedLT(sv, this, seen)) return isFeasable() && sv->isFeasable();
        lt.insert(sv);
        sv->gt.insert(this);
    }

    if (sv->isBoundedAbove())
    {
        clipUpperBound(sv->getUpperBound(), false);
        setRepeatLowerBound(sv->getUpperBound(), false);
    }
    else removeRepeatLowerBound();
    if (isBoundedBelow())
    {
        sv->clipLowerBound(getLowerBound());
        sv->setRepeatLowerBound(getLowerBound());
    }
    else sv->removeRepeatLowerBound();

    return isFeasable() && sv->isFeasable();
}

bool SymbolicVariable::addLE(const VarWrapper* vg, SymbolicExecutionFringe* sef,  bool constructed)
{
    GottenVarPtr<SymbolicVariable> other = vg->getSymbolicVariable(sef);
    SymbolicVariable* sv = other.get();

    if (!constructed && !other.constructed())
    {
        setRepeatUpperBound(sv->getUpperBound());
        sv->setRepeatLowerBound(getLowerBound());
        set<SymbolicVariable*> seen;
        if (guaranteedLE(sv, this, seen)) return isFeasable() && sv->isFeasable();
        lt.insert(sv);
        sv->gt.insert(this);
    }
    if (sv->isBoundedAbove())
    {
        clipUpperBound(sv->getUpperBound());
        setRepeatUpperBound(sv->getUpperBound());
    }
    else removeRepeatUpperBound();
    if (isBoundedBelow())
    {
        sv->clipLowerBound(getLowerBound());
        sv->setRepeatLowerBound(getLowerBound());
    }
    else sv->removeRepeatLowerBound();

    return isFeasable() && sv->isFeasable();
}

bool SymbolicVariable::addEQ(const VarWrapper* vg, SymbolicExecutionFringe* sef,  bool constructed)
{
    GottenVarPtr<SymbolicVariable> other = vg->getSymbolicVariable(sef);
    SymbolicVariable* sv = other.get();
    if (!other.constructed() && !constructed)
    {
        set<SymbolicVariable*> seen;
        if (guaranteedEQ(sv, this, seen)) return isFeasable() && sv->isFeasable();
        eq.insert(sv);
        sv->eq.insert(this);
    }

    if (sv->isDetermined()) setConstValue(sv->getConstString());
    else if (isDetermined()) sv->setConstValue(getConstString());
    else
    {
        if (sv->isBoundedAbove())
        {
            clipUpperBound(sv->getUpperBound());
            setRepeatUpperBound(sv->getUpperBound());
        }
        if (sv->isBoundedBelow())
        {
            clipLowerBound(sv->getLowerBound());
            setRepeatLowerBound(sv->getLowerBound());
        }
        if (isBoundedAbove())
        {
            sv->clipUpperBound(getUpperBound());
            sv->setRepeatUpperBound(getUpperBound());
        }
        if (isBoundedBelow())
        {
            sv->clipLowerBound(getLowerBound());
            sv->setRepeatLowerBound(getLowerBound());
        }
    }

    return isFeasable() && sv->isFeasable();
}

bool SymbolicVariable::addNEQ(const VarWrapper* vg, SymbolicExecutionFringe* sef,  bool constructed)
{
    GottenVarPtr<SymbolicVariable> other = vg->getSymbolicVariable(sef);
    SymbolicVariable* sv = other.get();
    if (!other.constructed())
    {
        if (find(neq.begin(), neq.end(), sv) != neq.end()) return isFeasable() && sv->isFeasable();
        neq.insert(sv);
    }
    if (!constructed) sv->neq.insert(this);
    return isFeasable() && sv->isFeasable();
}

bool SymbolicVariable::guaranteedLT(SymbolicVariable* searchFor, SymbolicVariable* searchInit, set<SymbolicVariable*>& seen)
{
    auto addLT = [&, this, searchFor] () -> void
    {
        lt.insert(searchFor);
        searchFor->gt.insert(this);
    };
    
    if (this == searchInit) return false;
    else if (searchFor == this) return true;
    
    if (canMeet(Relations::LT, searchFor) == MeetEnum::MUST)
    {
        addLT();
        return true;
    }
    for (SymbolicVariable* optr : lt)
    {
        if (optr->getName() == searchFor->getName() || optr->guaranteedLE(searchFor, searchInit, seen))
        {
            addLT();
            return true;
        }
    }
    for (SymbolicVariable* optr : le) if (optr->guaranteedLT(searchFor, searchInit, seen))
    {
        addLT();
        return true;
    }
    for (SymbolicVariable* optr : eq) if (optr->guaranteedLT(searchFor, searchInit, seen))
    {
        addLT();
        return true;
    }
    return false;
}

bool SymbolicVariable::guaranteedLE(SymbolicVariable* searchFor, SymbolicVariable* searchInit, set<SymbolicVariable*>& seen)
{
    auto addLE = [&, this, searchFor] () -> void
    {
        le.insert(searchFor);
        searchFor->ge.insert(this);
    };

    if (searchFor == this || searchInit == this) return true;
    else if (canMeet(Relations::LE, searchFor) == MeetEnum::MUST)
    {
        addLE();
        return true;
    }
    for (SymbolicVariable* optr : lt)
    {
        if (optr->getName() == searchFor->getName() || optr->guaranteedLE(searchFor, searchInit, seen))
        {
            addLE();
            return true;
        }
    }
    for (SymbolicVariable* optr : le) if (optr->guaranteedLE(searchFor, searchInit, seen))
    {
        addLE();
        return true;
    }
    for (SymbolicVariable* optr : eq) if (optr->guaranteedLE(searchFor, searchInit, seen))
    {
        addLE();
        return true;
    }
    return false;
}

bool SymbolicVariable::guaranteedGT(SymbolicVariable* searchFor, SymbolicVariable* searchInit, set<SymbolicVariable*>& seen)
{
    if (this == searchInit) return false;
    return searchFor->guaranteedLT(this, searchFor, seen);
}

bool SymbolicVariable::guaranteedGE(SymbolicVariable* searchFor, SymbolicVariable* searchInit, set<SymbolicVariable*>& seen)
{
    if (this == searchInit) return true;
    return searchFor->guaranteedLE(this, searchFor, seen);
}

bool SymbolicVariable::guaranteedEQ(SymbolicVariable* searchFor, SymbolicVariable* searchInit, set<SymbolicVariable*>& seen)
{
    if (this == searchInit || this == searchFor) return true;
    if (canMeet(Relations::EQ, searchFor) == MeetEnum::MUST)
    {
        eq.insert(searchFor);
        searchFor->eq.insert(this);
        return true;
    }
    else for (auto& equal : eq) if (equal->guaranteedEQ(searchFor, searchInit, seen)) return true;
    return false;
}

bool SymbolicVariable::guaranteedNEQ(SymbolicVariable* searchFor, SymbolicVariable* searchInit, set<SymbolicVariable*>& seen)
{
    if (this == searchInit || this == searchFor) return false;

    auto addNEQ = [&, searchFor, this] () -> void
    {
        neq.insert(searchFor);
        searchFor->neq.insert(this);
    };

    for (SymbolicVariable* optr : neq) if (optr->guaranteedEQ(searchFor, searchInit, seen))
        {addNEQ(); return true;}
    for (SymbolicVariable* optr : gt) if (optr->guaranteedGE(searchFor, searchInit, seen))
        {addNEQ(); return true;}
    for (SymbolicVariable* optr : ge) if (optr->guaranteedGT(searchFor, searchInit, seen))
        {addNEQ(); return true;}
    for (SymbolicVariable* optr : lt) if (optr->guaranteedLE(searchFor, searchInit, seen))
        {addNEQ(); return true;}
    for (SymbolicVariable* optr : le) if (optr->guaranteedLE(searchFor, searchInit, seen))
        {addNEQ(); return true;}
    for (SymbolicVariable* optr : eq) if (optr->guaranteedNEQ(searchFor, searchInit, seen))
        {addNEQ(); return true;} 
    return false;
}

void SymbolicVariable::clearEQ()
{
    for (auto& ptr : eq) ptr->eq.erase(this);
    eq.clear();
    for (auto& ptr : neq) ptr->neq.erase(this);
    neq.clear();
}

void SymbolicVariable::clearLess()
{
    for (auto& ptr : lt) ptr->lt.erase(this);
    lt.clear();
    for (auto& ptr : le) ptr->ge.erase(this);
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
    for (auto& ptr : gt) ptr->lt.erase(this);
    gt.clear();
    for (auto& ptr : ge) ptr->le.erase(this);
    ge.clear();
}

//SymbolicVariableTemplate
template <typename T>
SymbolicVariableTemplate<T>::SymbolicVariableTemplate(string name, const T lower, const T upper, const T repeatLow, const T repeatUp,
                                      Reporter& r, VariableType t, bool init, bool incrementable):
        SymbolicVariable(name, t, r, init, lower <= upper, incrementable), lowerBound(lower),
        upperBound(upper), repeatLower(repeatLow), repeatUpper(repeatUp) {}

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
SymbolicVariable::MeetEnum SymbolicVariableTemplate<T>::canMeet(Relations::Relop rel, SymbolicVariable* rhs) const
{
    throw std::runtime_error("not implemented for generic vars");
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
    neqConsts.insert(c);
}

template <>
void SymbolicVariableTemplate<double>::addNEQConst(const string& d)
{
    double c = stod(d);
    neqConsts.insert(c);
}

template <typename T>
bool SymbolicVariableTemplate<T>::addEQ(const VarWrapper* vg,
                                        SymbolicExecution::SymbolicExecutionFringe* sef, bool constructed)
{
    GottenVarPtr<SymbolicVariable> other = vg->getSymbolicVariable(sef);
    SymbolicVariable::addEQ(vg, sef, constructed);
    SymbolicVariable* sv = other.get();
    SymbolicVariableTemplate<T>* otherCast = static_cast<SymbolicVariableTemplate<T>*>(sv);
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
    if (isDetermined() && cv == getTConstValue()) return;
    if (cv >= upperBound) clearGreater();
    else if (cv <= lowerBound) clearLess();
    else clearAll();
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

template <typename T>
const T& SymbolicVariableTemplate<T>::getTRepeatLowerBound() const
{
    return repeatLower;
}

template <typename T>
const T& SymbolicVariableTemplate<T>::getTRepeatUpperBound() const
{
    return repeatUpper;
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
    if (!isDetermined()) throw std::runtime_error("Not constant");
    return lowerBound; //could be upper
}

template <typename T>
bool SymbolicVariableTemplate<T>::isDetermined() const
{
    return lowerBound == upperBound;
}

template class SymbolicVariableTemplate<double>;
template class SymbolicVariableTemplate<string>;