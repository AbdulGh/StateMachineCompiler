//
// Created by abdul on 14/08/17.
//
#include <limits>
#include <cmath>
#include "SymbolicDouble.h"
#include "SymbolicExecution.h"

//todo uniformly changing when set/clip/union is called
//todo update repeat bounds on arithmetic ops

using namespace std;
using namespace SymbolicExecution;

SymbolicDouble::SymbolicDouble(string name, Reporter& r):
    varN(move(name)), reporter(r), upperBound(0), lowerBound(0), repeatLower(numeric_limits<double>::lowest()),
    repeatUpper(numeric_limits<double>::max()) {}

SymbolicDouble::SymbolicDouble(const SymbolicDouble& o):
    varN(o.varN), reporter(o.reporter), upperBound(o.upperBound), lowerBound(o.lowerBound), repeatLower(o.repeatLower),
    repeatUpper(o.repeatUpper), minChange(o.minChange), maxChange(o.maxChange), defined(o.defined)
{}

SymbolicDouble& SymbolicDouble::operator=(const SymbolicDouble& o)
{
    varN = o.varN; upperBound = o.upperBound; lowerBound = o.lowerBound; repeatLower = o.repeatLower;
    repeatUpper = o.repeatUpper; minChange = o.minChange; maxChange = o.maxChange;
    defined = o.defined;
    return *this;
}

const string& SymbolicDouble::getName() const
{
    return varN;
}

void SymbolicDouble::setName(const string& newName)
{
    varN = newName;
}

void SymbolicDouble::reportError(Reporter::AlertType type, string err, int linenum)
{
    reporter.error(type, err, linenum);
    feasable = false;
}

bool SymbolicDouble::isDefined() const
{
    return defined;
}

void SymbolicDouble::define()
{
    defined = true;
}

bool SymbolicDouble::isFeasable()
{
    return feasable;
}

double SymbolicDouble::getLowerBound() const
{
    return lowerBound;
}

double SymbolicDouble::getUpperBound() const
{
    return upperBound;
}

double SymbolicDouble::getRepeatLowerBound() const
{
    return repeatLower;
}

double SymbolicDouble::getRepeatUpperBound() const
{
    return repeatUpper;
}


bool SymbolicDouble::isDetermined() const
{
    return lowerBound == upperBound;
}

double SymbolicDouble::getConstValue() const
{
    if (!isDetermined()) throw runtime_error("asked to get cvalue of indetermined var");
    return lowerBound;
}

void SymbolicDouble::removeRepeatUpperBound()
{
    repeatUpper = numeric_limits<double>::max();
}

void SymbolicDouble::removeRepeatLowerBound()
{
    repeatLower = numeric_limits<double>::lowest();
}

bool SymbolicDouble::addGT(const VarWrapper* vg, SymbolicExecutionFringe* sef, bool constructed)
{
    GottenVarPtr<SymbolicDouble> other = vg->getSymbolicDouble(sef, -1);
    SymbolicDouble* sd = other.get();

    if (sd->isBoundedBelow())
    {
        clipLowerBound(sd->getLowerBound(), 1);
        setRepeatLowerBound(sd->repeatLower, 1);
    }
    else removeRepeatLowerBound();

    if (isBoundedAbove())
    {
        sd->clipUpperBound(getUpperBound(), -1);
        sd->setRepeatLowerBound(repeatUpper, -1);
    }
    else sd->removeRepeatUpperBound();
    return isFeasable() && sd->isFeasable();
}

bool SymbolicDouble::addGE(const VarWrapper* vg, SymbolicExecutionFringe* sef,  bool constructed)
{
    GottenVarPtr<SymbolicDouble> other = vg->getSymbolicDouble(sef, -1);
    SymbolicDouble* sd = other.get();
    if (sd->isBoundedBelow())
    {
        clipLowerBound(sd->getLowerBound());
        setRepeatLowerBound(sd->repeatLower);
    }
    else removeRepeatLowerBound();
    if (isBoundedAbove())
    {
        sd->clipUpperBound(getUpperBound());
        sd->setRepeatUpperBound(repeatUpper);
    }
    else sd->removeRepeatUpperBound();
    return isFeasable() && sd->isFeasable();
}

bool SymbolicDouble::addLT(const VarWrapper* vg, SymbolicExecutionFringe* sef,  bool constructed)
{
    GottenVarPtr<SymbolicDouble> other = vg->getSymbolicDouble(sef, -1);
    SymbolicDouble* sd = other.get();
    if (sd->isBoundedAbove())
    {
        clipUpperBound(sd->getUpperBound(), -1);
        setRepeatUpperBound(sd->repeatUpper, -1);
    }
    else removeRepeatUpperBound();
    if (isBoundedBelow())
    {
        sd->clipLowerBound(getLowerBound());
        sd->setRepeatLowerBound(repeatLower);
    }
    else sd->removeRepeatLowerBound();

    return isFeasable() && sd->isFeasable();
}

bool SymbolicDouble::addLE(const VarWrapper* vg, SymbolicExecutionFringe* sef,  bool constructed)
{
    GottenVarPtr<SymbolicDouble> other = vg->getSymbolicDouble(sef, -1);
    SymbolicDouble* sd = other.get();
    if (sd->isBoundedAbove())
    {
        clipUpperBound(sd->getUpperBound());
        setRepeatUpperBound(sd->repeatUpper);
    }
    else removeRepeatUpperBound();
    if (isBoundedBelow())
    {
        sd->clipLowerBound(getLowerBound());
        sd->setRepeatLowerBound(repeatLower);
    }
    else sd->removeRepeatLowerBound();

    return isFeasable() && sd->isFeasable();
}

bool SymbolicDouble::addEQ(const VarWrapper* vg, SymbolicExecutionFringe* sef,  bool constructed)
{
    GottenVarPtr<SymbolicDouble> other = vg->getSymbolicDouble(sef, -1);
    SymbolicDouble* sd = other.get();
    if (sd->isDetermined()) setConstValue(sd->getConstValue());
    else if (isDetermined()) sd->setConstValue(getConstValue());
    else
    {
        if (sd->isBoundedAbove())
        {
            clipUpperBound(sd->getUpperBound());
            setRepeatUpperBound(sd->repeatUpper);
        }
        else removeRepeatUpperBound();

        if (sd->isBoundedBelow())
        {
            clipLowerBound(sd->getLowerBound());
            setRepeatLowerBound(sd->repeatLower);
        }
        else removeRepeatLowerBound();

        if (isBoundedAbove())
        {
            sd->clipUpperBound(getUpperBound());
            sd->setRepeatUpperBound(getUpperBound());
        }
        else sd->removeRepeatUpperBound();

        if (isBoundedBelow())
        {
            sd->clipLowerBound(getLowerBound());
            sd->setRepeatLowerBound(getLowerBound());
        }
        else sd->removeRepeatUpperBound();
    }

    return isFeasable() && sd->isFeasable();
}

bool SymbolicDouble::addNEQ(const VarWrapper* vg, SymbolicExecutionFringe* sef,  bool constructed)
{
    GottenVarPtr<SymbolicDouble> other = vg->getSymbolicDouble(sef, -1);
    SymbolicDouble* sd = other.get();
    return isFeasable() && sd->isFeasable();
}

/*
bool SymbolicDouble::guaranteedLT(SymbolicDouble* searchFor, SymbolicDouble* searchInit, set<SymbolicDouble*>& seen)
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
    for (SymbolicDouble* optr : lt)
    {
        if (optr->getName() == searchFor->getName() || optr->guaranteedLE(searchFor, searchInit, seen))
        {
            addLT();
            return true;
        }
    }
    for (SymbolicDouble* optr : le)
    {
        if (optr->guaranteedLT(searchFor, searchInit, seen))
        {
            addLT();
            return true;
        }
    }
    for (SymbolicDouble* optr : eq)
    {
        if (optr->guaranteedLT(searchFor, searchInit, seen))
        {
            addLT();
            return true;
        }
    }
    return false;
}

bool SymbolicDouble::guaranteedLE(SymbolicDouble* searchFor, SymbolicDouble* searchInit, set<SymbolicDouble*>& seen)
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
    for (SymbolicDouble* optr : lt)
    {
        if (optr->getName() == searchFor->getName() || optr->guaranteedLE(searchFor, searchInit, seen))
        {
            addLE();
            return true;
        }
    }
    for (SymbolicDouble* optr : le)
    {
        if (optr->guaranteedLE(searchFor, searchInit, seen))
        {
            addLE();
            return true;
        }
    }
    for (SymbolicDouble* optr : eq)
    {
        if (optr->guaranteedLE(searchFor, searchInit, seen))
        {
            addLE();
            return true;
        }
    }
    return false;
}

bool SymbolicDouble::guaranteedGT(SymbolicDouble* searchFor, SymbolicDouble* searchInit, set<SymbolicDouble*>& seen)
{
    if (this == searchInit) return false;
    return searchFor->guaranteedLT(this, searchFor, seen);
}

bool SymbolicDouble::guaranteedGE(SymbolicDouble* searchFor, SymbolicDouble* searchInit, set<SymbolicDouble*>& seen)
{
    if (this == searchInit) return true;
    return searchFor->guaranteedLE(this, searchFor, seen);
}

bool SymbolicDouble::guaranteedEQ(SymbolicDouble* searchFor, SymbolicDouble* searchInit, set<SymbolicDouble*>& seen)
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

bool SymbolicDouble::guaranteedNEQ(SymbolicDouble* searchFor, SymbolicDouble* searchInit, set<SymbolicDouble*>& seen)
{
    if (this == searchInit || this == searchFor) return false;

    auto addNEQ = [&, searchFor, this] () -> void
    {
        neq.insert(searchFor);
        searchFor->neq.insert(this);
    };

    for (SymbolicDouble* optr : neq) if (optr->guaranteedEQ(searchFor, searchInit, seen))
        {addNEQ(); return true;}
    for (SymbolicDouble* optr : gt) if (optr->guaranteedGE(searchFor, searchInit, seen))
        {addNEQ(); return true;}
    for (SymbolicDouble* optr : ge) if (optr->guaranteedGT(searchFor, searchInit, seen))
        {addNEQ(); return true;}
    for (SymbolicDouble* optr : lt) if (optr->guaranteedLE(searchFor, searchInit, seen))
        {addNEQ(); return true;}
    for (SymbolicDouble* optr : le) if (optr->guaranteedLE(searchFor, searchInit, seen))
        {addNEQ(); return true;}
    for (SymbolicDouble* optr : eq) if (optr->guaranteedNEQ(searchFor, searchInit, seen))
        {addNEQ(); return true;}
    return false;
}

void SymbolicDouble::clearEQ()
{
    for (auto& ptr : eq) ptr->eq.erase(this);
    eq.clear();
    for (auto& ptr : neq) ptr->neq.erase(this);
    neq.clear();
}

void SymbolicDouble::clearLess()
{
    for (auto& ptr : lt) ptr->gt.erase(this);
    lt.clear();
    for (auto& ptr : le) ptr->ge.erase(this);
    le.clear();
}

void SymbolicDouble::clearGreater()
{
    for (auto& ptr : gt) ptr->lt.erase(this);
    gt.clear();
    for (auto& ptr : ge) ptr->le.erase(this);
    ge.clear();
}

void SymbolicDouble::clearAll()
{
    clearEQ();
    clearEQ();
    clearLess();
}*/

SymbolicDouble::SymbolicDouble(SymbolicDouble* other): SymbolicDouble(*other) {}

unique_ptr<SymbolicDouble> SymbolicDouble::clone()
{
    return make_unique<SymbolicDouble>(this);
}

void SymbolicDouble::userInput()
{
    upperBound = numeric_limits<double>::max();
    lowerBound = numeric_limits<double>::lowest();
    define();
    uniformlyChanging = false;
}

void SymbolicDouble::nondet()
{
    upperBound = numeric_limits<double>::max();
    lowerBound = numeric_limits<double>::lowest();
    define();
}

void SymbolicDouble::loopInit()
{
    minChange = maxChange = 0;
    userAffected = false;
    uniformlyChanging = true;
}

SymbolicDouble::MeetEnum SymbolicDouble::canMeet(Relations::Relop rel, SymbolicDouble* rhs) const
{
    if (rhs->isDetermined()) return canMeet(rel, rhs->getConstValue());

    switch(rel)
    {
        case Relations::EQ:
            if (getUpperBound() < rhs->getLowerBound() || (getLowerBound() > rhs->getUpperBound())) return CANT;
            return MAY;
        case Relations::NEQ:
            if (getUpperBound() < rhs->getLowerBound() || (getLowerBound() > rhs->getUpperBound())) return MUST;
            return MAY;
        case Relations::LE:
            if (getUpperBound() <= rhs->getLowerBound()) return MUST;
            else if (getLowerBound() > rhs->getUpperBound()) return CANT;
            return MAY;
        case Relations::LT:
            if (getUpperBound() < rhs->getLowerBound()) return MUST;
            else if (getLowerBound() >= rhs->getUpperBound()) return CANT;
            return MAY;
        case Relations::GE:
            if (getLowerBound() >= rhs->getUpperBound()) return MUST;
            else if (getUpperBound() < rhs->getLowerBound()) return CANT;
            return MAY;
        case Relations::GT:
            if (getLowerBound() > rhs->getUpperBound()) return MUST;
            else if (getUpperBound() <= rhs->getLowerBound()) return CANT;
            return MAY;
        default:
            throw std::runtime_error("weird enum");
    }
}

SymbolicDouble::MeetEnum SymbolicDouble::canMeet(Relations::Relop rel, double rhs) const
{
    switch(rel)
    {
        case Relations::EQ:
            if (isDetermined() && getLowerBound() == rhs) return MUST;
            else if (rhs >= getLowerBound() && rhs <= getUpperBound()) return MAY;
            else return CANT;
        case Relations::NEQ:
            if (isDetermined()) return (getLowerBound() != rhs) ? MUST : CANT;
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

bool SymbolicDouble::setLowerBound(double d, short direction)
{
    if (direction == -1 && d != numeric_limits<double>::lowest()) lowerBound = nextafter(d, numeric_limits<double>::lowest());
    else if (direction == 1 && d != numeric_limits<double>::max()) lowerBound = nextafter(d, numeric_limits<double>::max());
    else lowerBound = d;

    if (lowerBound > upperBound) feasable = false;
    return isFeasable();
}

bool SymbolicDouble::setUpperBound(double d, short direction)
{
    if (direction == -1 && d != numeric_limits<double>::lowest()) upperBound = nextafter(d, numeric_limits<double>::lowest());
    else if (direction == 1 && d != numeric_limits<double>::max()) upperBound = nextafter(d, numeric_limits<double>::max());
    else upperBound = d;

    if (lowerBound > upperBound) feasable = false;
    return isFeasable();
}

void SymbolicDouble::minLowerBound()
{
    lowerBound = repeatLower;
}
void SymbolicDouble::maxUpperBound()
{
    upperBound = repeatUpper;
}

void SymbolicDouble::removeLowerBound()
{
    lowerBound = numeric_limits<double>::lowest();
}
void SymbolicDouble::removeUpperBound()
{
    upperBound = numeric_limits<double>::max();
}

bool SymbolicDouble::clipLowerBound(double d, short direction)
{
    if (d > getLowerBound()) return setLowerBound(d, direction);
    else return isFeasable();
}

bool SymbolicDouble::clipUpperBound(double d, short direction)
{
    if (d < getUpperBound()) return setUpperBound(d, direction);
    else return isFeasable();
}

bool SymbolicDouble::unionLowerBound(double d, short direction)
{
    if (direction != 0)
    {
        if (direction == -1 && d != numeric_limits<double>::lowest()) d = nextafter(d, numeric_limits<double>::lowest());
        else if (direction == 1 && d != numeric_limits<double>::max()) d = nextafter(d, numeric_limits<double>::max());
        else throw runtime_error("direction in [-1, 1]");

        if (d < lowerBound)
        {
            lowerBound = d;
            return true;
        }
        else return false;
    }

    if (d < getLowerBound())
    {
        setLowerBound(d);
        return true;
    }
    else return false;
}

bool SymbolicDouble::unionUpperBound(double d, short direction)
{
    if (direction != 0)
    {
        if (direction == -1 && d != numeric_limits<double>::lowest()) d = nextafter(d, numeric_limits<double>::lowest());
        else if (direction == 1 && d != numeric_limits<double>::max()) d = nextafter(d, numeric_limits<double>::max());

        if (d > upperBound)
        {
            upperBound = d;
            return true;
        }
        else return false;
    }

    if (d > upperBound)
    {
        upperBound = d;
        return true;
    }
    else return false;
}

bool SymbolicDouble::unionVar(const SymbolicDouble* other)
{
    bool ret = unionLowerBound(other->getLowerBound());
    if (unionUpperBound(other->getUpperBound())) ret = true;
    if (other->minChange < minChange)
    {
        minChange = other->minChange;
        ret = true;
    }
    if (other->maxChange > maxChange)
    {
        maxChange = other->maxChange;
        ret = true;
    }
    return ret;
}

void SymbolicDouble::setConstValue(double d)
{
    lowerBound = upperBound = repeatLower = repeatUpper = d;
    defined = true;
    minChange = maxChange = 0;
    uniformlyChanging = false;
}

bool SymbolicDouble::unionConstValue(double cv, short direction)
{
    bool change = false;
    if (cv < lowerBound)
    {
        change = true;
        lowerBound = cv;
    }
    else if (cv > upperBound)
    {
        change = true;
        upperBound = cv;
    }
    return change;
}

void SymbolicDouble::setRepeatLowerBound(double lb, short direction)
{
    if (direction == -1 && lb != numeric_limits<double>::lowest()) lb = nextafter(lb, numeric_limits<double>::lowest());
    else if (direction == 1 && lb != numeric_limits<double>::max()) lb = nextafter(lb, numeric_limits<double>::max());
    repeatLower = lb;
}

void SymbolicDouble::resetRepeatBounds()
{
    repeatLower = numeric_limits<double>::lowest();
    repeatUpper = numeric_limits<double>::max();
}

void SymbolicDouble::setRepeatUpperBound(double ub, short direction)
{
    if (direction == -1 && ub != numeric_limits<double>::lowest()) ub = nextafter(ub, numeric_limits<double>::lowest());
    else if (direction == 1 && ub != numeric_limits<double>::max()) ub = nextafter(ub, numeric_limits<double>::max());
    repeatUpper = ub;
}

void SymbolicDouble::setRepeatBoundsFromComparison(Relations::Relop r, double d)
{
    switch(r)
    {
        case Relations::EQ:
            setRepeatLowerBound(d);
            setRepeatUpperBound(d);
            break;
        case Relations::NEQ:
            break;
        case Relations::LT:
            setRepeatUpperBound(d, -1);
            break;
        case Relations::LE:
            setRepeatUpperBound(d);
            break;
        case Relations::GT:
            setRepeatLowerBound(d, 1);
            break;
        case Relations::GE:
            setRepeatLowerBound(d);
            break;
        default:
            throw runtime_error("bad relop");
    }
}

void SymbolicDouble::setRepeatBoundsFromComparison(Relations::Relop r, SymbolicDouble *rhs)
{
    switch(r)
    {
        case Relations::EQ:
            setRepeatLowerBound(rhs->repeatLower);
            setRepeatUpperBound(rhs->repeatUpper);
            break;
        case Relations::NEQ:
            break;
        case Relations::LT:
            setRepeatUpperBound(rhs->repeatUpper, -1);
            break;
        case Relations::LE:
            setRepeatUpperBound(rhs->repeatUpper);
            break;
        case Relations::GT:
            setRepeatLowerBound(rhs->repeatLower, 1);
            break;
        case Relations::GE:
            setRepeatLowerBound(rhs->repeatLower);
            break;
        default:
            throw runtime_error("bad relop");
    }
}

void SymbolicDouble::iterateTo(double toD, short direction)
{
    if (toD < lowerBound)
    {
        if (minChange > 0) throw std::runtime_error("cant move downwards");
        if (numeric_limits<double>::lowest() - minChange > toD)
        {
            lowerBound = numeric_limits<double>::lowest();
        }
        else
        {
            lowerBound = toD + minChange;
            if (direction == -1 && lowerBound != numeric_limits<double>::lowest())
            {
                lowerBound = nextafter(lowerBound, numeric_limits<double>::lowest());
            }
            else if (direction == 1 && lowerBound != numeric_limits<double>::max())
            {
                lowerBound = nextafter(lowerBound, numeric_limits<double>::max());
            }
        }
    }
    else if (toD > upperBound)
    {
        if (maxChange < 0) throw std::runtime_error("cant move downwards");
        if (numeric_limits<double>::max() - maxChange < toD)
        {
            upperBound = numeric_limits<double>::max();
        }
        else
        {
            if (direction == -1 && upperBound != numeric_limits<double>::lowest())
            {
                upperBound = nextafter(upperBound, numeric_limits<double>::lowest());
            }
            else if (direction == 1 && upperBound != numeric_limits<double>::max())
            {
                upperBound = nextafter(upperBound, numeric_limits<double>::max());
            }
        }
    }
}

void SymbolicDouble::iterateTo(SymbolicDouble* sd, short direction) //todo check usages for direction
{
    if (upperBound <= sd->getLowerBound()) iterateTo(sd->getLowerBound(), direction);
    else if (lowerBound >= sd->getUpperBound()) iterateTo(sd->getUpperBound(), direction);
    else throw runtime_error("should have been one of those");
}

bool SymbolicDouble::getRelativeVelocity(SymbolicDouble* other, long double& slowest, long double& fastest) const
{
    fastest = maxChange - other->minChange;
    slowest = minChange - other->maxChange;
    return true;
}

bool SymbolicDouble::isBoundedAbove() const
{
    return upperBound != numeric_limits<double>::max();
}

bool SymbolicDouble::isBoundedBelow() const
{
    return upperBound != numeric_limits<double>::lowest();
}

SymbolicDouble::MonotoneEnum SymbolicDouble::getMonotonicity() const
{
    if (!uniformlyChanging) return NONE;
    else if (minChange > 0) return INCREASING;
    else if (maxChange < 0) return DECREASING;
    else if (minChange == 0 &&  maxChange == 0) return FRESH;
    else return UNKNOWN;
}

void SymbolicDouble::addConstToLower(const double diff, int linenum)
{
    if (!defined)
    {
        reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised", linenum);
    }

    if (!isBoundedBelow())
    {
        if (diff < 0) reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly drop below limit", linenum);
        else lowerBound = numeric_limits<double>::lowest() + diff; //kind of assuming diff is not too big
    }
    else
    {
        if (diff > 0)
        {
            if (lowerBound > numeric_limits<double>::max() - diff) reportError(Reporter::AlertType::RANGE, varN + " will overflow", linenum);
            else lowerBound += diff;
        }
        else
        {
            if (lowerBound < numeric_limits<double>::lowest() - diff)
            {
                reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly exceed double limits", linenum);
            }
            else lowerBound += diff;
        }
    }

    repeatUpper += diff;
}

void SymbolicDouble::addConstToUpper(const double diff, int linenum)
{
    if (!defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised", linenum);

    if (!isBoundedAbove())
    {
        if (diff > 0) reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly exceed double limits", linenum);
        else upperBound = numeric_limits<double>::max() + diff;
    }
    else
    {
        if (diff > 0)
        {
            if (upperBound > numeric_limits<double>::max() - diff)
            {
                reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly exceed double limits", linenum);
                upperBound = numeric_limits<double>::max();
            }
            else upperBound += diff;
        }
        else
        {
            if (upperBound < numeric_limits<double>::lowest() - diff) reportError(Reporter::AlertType::RANGE, varN + " will overflow", linenum);
            else upperBound += diff;
        }
    }

    repeatUpper += diff;
}

void SymbolicDouble::addConst(double diff, int linenum)
{
    if (!defined)
    {
        reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised", linenum);
    }

    if (diff == 0)
    {
        reporter.warn(Reporter::AlertType::USELESS_OP, "Zero added to " + varN, linenum);
        return;
    }

    if (!isDetermined())
    {
        addConstToLower(diff, linenum);
        addConstToUpper(diff, linenum);
    }

    else
    {
        double oldT = lowerBound;
        if (diff > 0 && oldT > numeric_limits<double>::max() - diff)
        {
            reportError(Reporter::AlertType::RANGE, varN + " will overflow", linenum);
        }
        else if (diff < 0 && oldT < numeric_limits<double>::lowest() - diff)
        {
            reportError(Reporter::AlertType::RANGE, varN + " will overflow", linenum);
        }
        upperBound = lowerBound = oldT + diff;
    }

    minChange += diff;
    maxChange += diff;
}

void SymbolicDouble::addSymbolicDouble(SymbolicDouble& other, int linenum, bool increment)
{
    if (!other.defined)
    {
        reporter.warn(Reporter::AlertType::UNINITIALISED_USE, other.varN + " used before explicitly initialised", linenum);
    }

    if (other.isDetermined())
    {
        addConst(other.getConstValue(), linenum);
        return;
    }

    if (!defined)
    {
        reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised", linenum);
    }

    double otherLowerBound = other.getLowerBound();
    double otherUpperBound = other.getUpperBound();

    addConstToLower(otherLowerBound, linenum);
    addConstToUpper(otherUpperBound, linenum);

    if (!increment)
    {
        minChange += other.minChange;
        maxChange += other.maxChange;
    }
    else
    {
        minChange += other.getLowerBound();
        maxChange += other.getUpperBound();
    }
}

void SymbolicDouble::minusSymbolicDouble(SymbolicDouble& other, int linenum, bool increment)
{
    if (!other.defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, other.varN + " used before explicitly initialised");

    if (other.isDetermined())
    {
        addConst(other.getConstValue(), linenum);
        return;
    }

    if (!defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

    double otherLowerBound = other.getLowerBound();
    double otherUpperBound = other.getUpperBound();

    addConstToLower(-otherUpperBound, linenum);
    addConstToUpper(-otherLowerBound, linenum);

    if (increment)
    {
        minChange -= otherUpperBound;
        maxChange -= otherLowerBound;
    }
    else
    {
        minChange += other.minChange;
        maxChange += other.maxChange;
    }
}

enum ArithResult{OVER, UNDER, FINE};
ArithResult safeMultiply(double a, double b, double& result)
{
    long double al = (long double) a;
    long double bl = (long double) b;

    if (signbit(a) == signbit(b))
    {
        long double ml = (long double)numeric_limits<double>::max();
        if (ml / abs(bl) < abs(al))
        {
            result = numeric_limits<double>::max();
            return ArithResult::OVER;
        }
    }
    else
    {
        long double ll = (long double)numeric_limits<double>::lowest();
        if (abs(ll) / abs(bl) < abs(al))
        {
            result = numeric_limits<double>::lowest();
            return ArithResult::UNDER;
        }
    }
    result = a * b;
    return ArithResult::FINE;
}

void SymbolicDouble::multConst(double mul, int linenum)
{
    double change1 = upperBound * (mul - 1);
    double change2 = lowerBound * (mul - 1);
    if (change1 > change2)
    {
        maxChange += change1;
        minChange += change2;
    }
    else
    {
        maxChange += change2;
        minChange += change1;
    }

    if (!defined)
    {
        reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised", linenum);
    }
    if (mul == 0) setConstValue(0);
    else if (mul == 1)
    {
        reporter.warn(Reporter::AlertType::USELESS_OP, varN + "multiplied by 1", linenum);
    }
    else
    {
        if (isDetermined())
        {
            double temp;
            ArithResult result = safeMultiply(getConstValue(), mul, temp);
            if (result != FINE)
            {
                reportError(Reporter::AlertType::RANGE,
                            to_string(getConstValue()) + " * " + to_string(mul) + " = overflow", linenum);
            }
            else setConstValue(temp);
        }
        else
        {
            double lowerResult, upperResult;

            bool bad = false;
            bool alwaysabove = true;
            bool alwaysbelow = true;

            ArithResult result = safeMultiply(lowerBound, mul, lowerResult);
            if (result == FINE) alwaysabove = alwaysbelow = false;
            else
            {
                bad = true;
                if (result == OVER) alwaysbelow = false;
                else alwaysabove = false;
            }

            result = safeMultiply(upperBound, mul, upperResult);
            if (result == FINE) alwaysabove = alwaysbelow = false;
            else
            {
                bad = true;
                if (result == OVER) alwaysbelow = false;
                else alwaysabove = false;
            }

            if (alwaysabove || alwaysbelow)
            {
                reportError(Reporter::AlertType::RANGE,
                            varN + " guaranteed to overflow when multiplied by " + to_string(mul), linenum);
            }
            else if (bad)
            {
                reporter.warn(Reporter::AlertType::RANGE,
                              varN + " might overflow when multiplied by " + to_string(mul), linenum);
            }

            if (lowerResult <= upperResult)
            {
                setLowerBound(lowerResult);
                setUpperBound(upperResult);
            }
            else
            {
                setLowerBound(upperResult);
                setUpperBound(lowerResult);
            }
        }
    }
}

void SymbolicDouble::multSymbolicDouble(SymbolicDouble& other, int linenum)
{
    if (!other.defined)
    {
        reporter.warn(Reporter::AlertType::UNINITIALISED_USE,
                      other.varN + " used before explicitly initialised", linenum);
    }
    if (other.isDetermined())
    {
        multConst(other.getConstValue(), linenum);
        return;
    }
    if (!defined)
    {
        reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised", linenum);
    }

    double otherLowerBound = other.getLowerBound();
    double otherUpperBound = other.getUpperBound();

    bool bad = false; //can overflow
    bool alwaysabove = true; //always overflow
    bool alwaysbelow = true; //always negative overflow

    double lowerlower, lowerupper, upperlower, upperupper;
    ArithResult result = safeMultiply(lowerBound, otherLowerBound, lowerlower);
    if (result == FINE)
    {
        alwaysabove = false;
        alwaysbelow = false;
    }
    else
    {
        bad = true;
        if (result == OVER) alwaysbelow = false;
        else alwaysabove = false;
    }

    result = safeMultiply(lowerBound, otherUpperBound, lowerupper);
    if (result == FINE)
    {
        alwaysabove = false;
        alwaysbelow = false;
    }
    else
    {
        bad = true;
        if (result == OVER) alwaysbelow = false;
        else alwaysabove = false;
    }

    result = safeMultiply(upperBound, otherLowerBound, upperlower);
    if (result == FINE)
    {
        alwaysabove = false;
        alwaysbelow = false;
    }
    else
    {
        bad = true;
        if (result == OVER) alwaysbelow = false;
        else alwaysabove = false;
    }

    result = safeMultiply(upperBound, otherUpperBound, upperupper);
    if (result == FINE)
    {
        alwaysabove = false;
        alwaysbelow = false;
    }
    else
    {
        bad = true;
        if (result == OVER) alwaysbelow = false;
        else alwaysabove = false;
    }

    if (alwaysabove || alwaysbelow)
    {
        reportError(Reporter::AlertType::RANGE,
                    varN + " guaranteed to overflow when multiplied by " + other.varN, linenum);
    }
    else if (bad)
    {
        reporter.warn(Reporter::AlertType::RANGE,
                      varN + " might overflow when multiplied by " + other.varN, linenum);
    }


    double oldLower = lowerBound;
    double oldUpper = upperBound;

    setLowerBound(min(lowerlower, min(lowerupper, min(upperlower, upperupper))));
    setUpperBound(max(lowerlower, max(lowerupper, max(upperlower, upperupper))));

    //todo make this more accurate
    minChange += upperBound - oldUpper;
    maxChange += lowerBound - oldLower;
}

void SymbolicDouble::modConst(double modulus, int linenum)
{
    if (modulus == 0)
    {
        reportError(Reporter::AlertType::ZERODIVISION, varN + " divided by zero", linenum);
        return;
    }

    if (lowerBound < 0 || upperBound > modulus) uniformlyChanging = false;
    if (isDetermined())
    {
        setConstValue(fmod(getConstValue(),modulus));
        return;
    }
    else
    {
        setLowerBound(0);
        modulus = abs(modulus);
        if (upperBound >= 0) setUpperBound(min(upperBound, modulus));
        else setUpperBound(modulus);
    }
}

void SymbolicDouble::modSymbolicDouble(SymbolicDouble& other, int linenum)
{
    if (other.isDetermined())
    {
        double otherVal = other.getConstValue();
        if (otherVal == 0)
        {
            reportError(Reporter::ZERODIVISION, varN + " divided by " + other.varN + " ( = 0)", linenum);
            return;
        }
        else modConst(otherVal, linenum);
    }

    else
    {
        if (lowerBound < 0 || upperBound > other.lowerBound) uniformlyChanging = false;
        setLowerBound(0);
        if (other.lowerBound <= 0 && other.upperBound >= 0)
        {
            reporter.warn(Reporter::ZERODIVISION,
                          varN + " divided by " + other.varN + " which could possibly be zero", linenum);
        }
        setUpperBound(max(upperBound, max(abs(other.lowerBound), abs(other.upperBound))));
    }
}

ArithResult safeDivide(double a, double b, double& result)
{
    if (abs(b) >= 1)
    {
        result = a / b;
        return ArithResult::FINE;
    }
    else if (b > 0 && a > b * numeric_limits<double>::max())
    {
        result = numeric_limits<double>::max();
        return ArithResult::OVER;
    }
    else if (b < 0 && a < b * numeric_limits<double>::lowest())
    {
        result = numeric_limits<double>::lowest();
        return ArithResult::UNDER;
    }
    result = a/b;
    return ArithResult::FINE;
}

void SymbolicDouble::divConst(double denom, int linenum)
{
    if (!defined)
    {
        reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised", linenum);
    }

    double change1 = upperBound * (1/denom - 1);
    double change2 = lowerBound * (1/denom - 1);
    if (change1 > change2)
    {
        maxChange += change1;
        minChange += change2;
    }
    else
    {
        maxChange += change2;
        minChange += change1;
    }

    if (denom == 0)
    {
        reportError(Reporter::AlertType::ZERODIVISION, varN + "divided by 0", linenum);
        return;
    }
    else if (denom == 1)
    {
        reporter.warn(Reporter::AlertType::USELESS_OP, varN + "divided by 1", linenum);
        return;
    }
    else if (isDetermined())
    {
        double temp;
        ArithResult result = safeDivide(getConstValue(), denom, temp);
        if (result != FINE)
        {
            reportError(Reporter::AlertType::RANGE, to_string(getConstValue()) + " / " + to_string(denom) + " = overflow", linenum);
        }
        else setConstValue(temp);
        return;
    }
    else
    {
        double oldUpper = upperBound;
        double oldLower = lowerBound;
        double lowerResult, upperResult;

        bool bad = false;
        bool alwaysabove = true;
        bool alwaysbelow = true;

        ArithResult result = safeDivide(oldLower, denom, lowerResult);
        if (result == FINE) alwaysabove = alwaysbelow = false;
        else
        {
            bad = true;
            if (result == OVER) alwaysbelow = false;
            else alwaysabove = false;
        }

        result = safeDivide(oldUpper, denom, upperResult);
        if (result == FINE) alwaysabove = alwaysbelow = false;
        else
        {
            bad = true;
            if (result == OVER) alwaysbelow = false;
            else alwaysabove = false;
        }

        if (alwaysabove || alwaysbelow)
        {
            reportError(Reporter::AlertType::RANGE, varN + " guaranteed to overflow when divided by " + to_string(denom), linenum);
        }
        else if (bad)
        {
            reporter.warn(Reporter::AlertType::RANGE, varN + " might overflow when divided by " + to_string(denom), linenum);
        }

        if (lowerResult <= upperResult)
        {
            setLowerBound(lowerResult);
            setUpperBound(upperResult);
        }
        else
        {
            setLowerBound(upperResult);
            setUpperBound(lowerResult);
        }

    }
}

void SymbolicDouble::divSymbolicDouble(SymbolicDouble& other, int linenum)
{
    if (!other.defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, other.varN + " used before explicitly initialised");
    if (other.isDetermined())
    {
        divConst(other.getConstValue(), linenum);
        return;
    }
    if (!defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

    double otherLowerBound = other.getLowerBound();
    double otherUpperBound = other.getUpperBound();

    double oldUpper = upperBound;
    double oldLower = lowerBound;

    bool bad = false; //can overflow
    bool alwaysabove = true; //always overflow
    bool alwaysbelow = true; //always negative overflow

    double lowerlower, lowerupper, upperlower, upperupper;
    ArithResult result = safeDivide(lowerBound, otherLowerBound, lowerlower);
    if (result == FINE) alwaysabove = alwaysbelow = false;
    else
    {
        bad = true;
        if (result == OVER) alwaysbelow = false;
        else alwaysabove = false;
    }

    result = safeDivide(lowerBound, otherUpperBound, lowerupper);
    if (result == FINE) alwaysabove = alwaysbelow = false;
    else
    {
        bad = true;
        if (result == OVER) alwaysbelow = false;
        else alwaysabove = false;
    }

    result = safeDivide(upperBound, otherLowerBound, upperlower);
    if (result == FINE) alwaysabove = alwaysbelow = false;
    else
    {
        bad = true;
        if (result == OVER) alwaysbelow = false;
        else alwaysabove = false;
    }

    result = safeDivide(upperBound, otherUpperBound, upperupper);
    if (result == FINE) alwaysabove = alwaysbelow = false;
    else
    {
        bad = true;
        if (result == OVER) alwaysbelow = false;
        else alwaysabove = false;
    }

    if (alwaysabove || alwaysbelow)
    {
        reportError(Reporter::AlertType::RANGE, varN + " guaranteed to overflow when divided by " + other.varN, linenum);
    }
    else if (bad)
    {
        reporter.warn(Reporter::AlertType::RANGE, varN + " might overflow when divided by " + other.varN, linenum);
    }

    setLowerBound(min(lowerlower, min(lowerupper, min(upperlower, upperupper))));
    setUpperBound(max(lowerlower, max(lowerupper, max(upperlower, upperupper))));

    minChange += upperBound - oldUpper;
    maxChange += lowerBound - oldLower;
}
