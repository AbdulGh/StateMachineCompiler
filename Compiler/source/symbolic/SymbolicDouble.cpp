//
// Created by abdul on 14/08/17.
//
#include <limits>
#include <cmath>
#include "SymbolicVariables.h"

//todo uniformly changing when set/clip/union is called

using namespace std;

enum ArithResult{OVER, UNDER, FINE};

SymbolicDouble::SymbolicDouble(string name, Reporter& r):
        SymbolicVariableTemplate(move(name), 0, 0,
                                 numeric_limits<double>::lowest(), numeric_limits<double>::max(),
                                 r, DOUBLE, true, true) {}

SymbolicDouble::SymbolicDouble(SymbolicDouble& o):
        SymbolicVariableTemplate
                (o.getName(), o.getTLowerBound(), o.getTUpperBound(),
                 o.getTRepeatLowerBound(), o.getTRepeatUpperBound(), o.reporter, DOUBLE, o.defined, true),
        minChange(o.minChange), maxChange(o.maxChange), uniformlyChanging(o.uniformlyChanging) {}

SymbolicDouble::SymbolicDouble(SymbolicVariable* other): SymbolicDouble(*static_cast<SymbolicDouble*>(other)) {}

unique_ptr<SymbolicVariable> SymbolicDouble::clone()
{
    return make_unique<SymbolicDouble>(this);
}

unique_ptr<SymbolicDouble> SymbolicDouble::cloneSD()
{
    return make_unique<SymbolicDouble>(this);
}

SymbolicVariable* SymbolicDouble::cloneRaw()
{
    return new SymbolicDouble(this);
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

SymbolicVariable::MeetEnum SymbolicDouble::canMeet(Relations::Relop rel, SymbolicVariable* r) const
{
    if (r->isDetermined()) return canMeet(rel, r->getConstString());
    SymbolicDouble* rhs = static_cast<SymbolicDouble*>(r);

    switch(rel)
    {
        case Relations::EQ:
            if (getTUpperBound() < rhs->getTLowerBound() || (getTLowerBound() > rhs->getTUpperBound())) return CANT;
            return MAY;
        case Relations::NEQ:
            if (getTUpperBound() < rhs->getTLowerBound() || (getTLowerBound() > rhs->getTUpperBound())) return MUST;
            return MAY;
        case Relations::LE:
            if (getTUpperBound() <= rhs->getTLowerBound()) return MUST;
            else if (getTLowerBound() > rhs->getTUpperBound()) return CANT;
            return MAY;
        case Relations::LT:
            if (getTUpperBound() < rhs->getTLowerBound()) return MUST;
            else if (getTLowerBound() >= rhs->getTUpperBound()) return CANT;
            return MAY;
        case Relations::GE:
            if (getTLowerBound() >= rhs->getTUpperBound()) return MUST;
            else if (getTUpperBound() < rhs->getTLowerBound()) return CANT;
            return MAY;
        case Relations::GT:
            if (getTLowerBound() > rhs->getTUpperBound()) return MUST;
            else if (getTUpperBound() <= rhs->getTLowerBound()) return CANT;
            return MAY;
        default:
            throw std::runtime_error("weird enum");
    }
}

SymbolicVariable::MeetEnum SymbolicDouble::canMeet(Relations::Relop rel, const std::string& rhstring) const
{
    double rhs = stod(rhstring);
    switch(rel)
    {
        case Relations::EQ:
            if (isDetermined() && getTLowerBound() == rhs) return MUST;
            else if (rhs >= getTLowerBound() && rhs <= getTUpperBound()) return MAY;
            else return CANT;
        case Relations::NEQ:
            if (isDetermined()) return (getTLowerBound() != rhs) ? MUST : CANT;
            else return MAY;
        case Relations::LE:
            if (getTUpperBound() <= rhs) return MUST;
            else if (getTLowerBound() > rhs) return CANT;
            return MAY;
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
    }
}

bool SymbolicDouble::setTLowerBound(const double& d, bool closed)
{
    if (d < lowerBound) clearGreater();
    if (!closed && d != numeric_limits<double>::lowest()) lowerBound = nextafter(d, numeric_limits<double>::lowest());
    else lowerBound = d;

    if (lowerBound > upperBound) feasable = false;
    return isFeasable();
}
bool SymbolicDouble::setLowerBound(const std::string& lb, bool closed)
{
    setTLowerBound(stod(lb), closed);
}

bool SymbolicDouble::setTUpperBound(const double& d, bool closed)
{
    if (d > upperBound) clearLess();
    if (!closed && d != numeric_limits<double>::max()) upperBound = nextafter(d, numeric_limits<double>::max());
    else upperBound = d;

    if (lowerBound > upperBound) feasable = false;
    return isFeasable();
}
bool SymbolicDouble::setUpperBound(const std::string& ub, bool closed)
{
    return setTUpperBound(stod(ub), closed);
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

bool SymbolicDouble::clipTLowerBound(const double& d, bool closed)
{
    if (d > getTLowerBound()) return setTLowerBound(d, closed);
    else return isFeasable();
}
bool SymbolicDouble::clipUpperBound(const std::string& ub, bool closed)
{
    return clipTUpperBound(stod(ub), closed);
}

bool SymbolicDouble::clipTUpperBound(const double& d, bool closed)
{
    if (d < getTUpperBound()) return setTUpperBound(d, closed);
    else return isFeasable();
}
bool SymbolicDouble::clipLowerBound(const std::string& lb, bool closed)
{
    return clipTLowerBound(stod(lb), closed);
}

bool SymbolicDouble::unionTLowerBound(const double& d, bool closed)
{
    if (closed && d < getTLowerBound()
        || !closed && d <= getTLowerBound())
    {
        setTLowerBound(d, closed);
        return true;
    }
    else return false;
}
bool SymbolicDouble::unionUpperBound(const std::string& ub, bool closed)
{
    return unionTUpperBound(stod(ub), closed);
}

bool SymbolicDouble::unionTUpperBound(const double& d, bool closed)
{
    if (closed && d > getTUpperBound()
        || !closed && d >= getTUpperBound())
    {
        setTUpperBound(d, closed);
        return true;
    }
    else return false;
}
bool SymbolicDouble::unionLowerBound(const std::string& lb, bool closed)
{
    return unionTLowerBound(stod(lb), closed);
}

bool SymbolicDouble::unionVar(const SymbolicVariable* other)
{
    if (other->getType() != DOUBLE) throw std::runtime_error("wrong");
    const SymbolicDouble* sd = static_cast<const SymbolicDouble*>(other);
    bool ret = unionTLowerBound(sd->getTLowerBound());
    if (unionTUpperBound(sd->getTUpperBound())) ret = true;
    return ret;
}

void SymbolicDouble::setTConstValue(const double& d)
{
    clearAll();
    SymbolicVariableTemplate<double>::setTConstValue(d);
    uniformlyChanging = false;
}
void SymbolicDouble::unionTConstValue(const double& cv, bool closed)
{
    clearAll();
    if (closed)
    {
        if (cv < lowerBound) lowerBound = cv;
        else if (cv > upperBound) upperBound = cv;
    }
    else
    {
        if (cv != numeric_limits<double>::lowest() && cv <= lowerBound)
        {
            lowerBound = nextafter(cv, numeric_limits<double>::lowest());
        }
        else if (cv != numeric_limits<double>::max() && cv >= upperBound)
        {
            lowerBound = nextafter(cv, numeric_limits<double>::max());
        }
    }
}
void SymbolicDouble::setConstValue(const std::string& c)
{
    clearAll();
    double cd = stod(c);
    setTConstValue(cd);
}

void SymbolicDouble::clipTRepeatLowerBound(const double& lb, bool closed)
{
    if (!closed)
    {
        double d = (lb == numeric_limits<double>::lowest() ?
                       numeric_limits<double>::lowest() : nextafter(lb, numeric_limits<double>::lowest()));
        repeatLower = max(d, repeatLower);
    }
    else repeatLower = max(lb, repeatLower);
}
void SymbolicDouble::clipRepeatLowerBound(const std::string& lb, bool closed)
{
    clipTRepeatLowerBound(stod(lb), closed);
}

void SymbolicDouble::setTRepeatLowerBound(const double& lb, bool closed)
{
    if (!closed)
    {
        repeatLower = (lb == numeric_limits<double>::lowest() ?
                      numeric_limits<double>::lowest() : nextafter(lb, numeric_limits<double>::lowest()));
    }
    else repeatLower = lb;
}
void SymbolicDouble::setRepeatLowerBound(const std::string& lb, bool closed)
{
    setTRepeatLowerBound(stod(lb), closed);
}

void SymbolicDouble::resetRepeatBounds()
{
    repeatLower = numeric_limits<double>::lowest();
    repeatUpper = numeric_limits<double>::max();
}

void SymbolicDouble::clipTRepeatUpperBound(const double& ub, bool closed)
{
    if (!closed)
    {
        double d = (ub == numeric_limits<double>::max() ?
                    numeric_limits<double>::max() : nextafter(ub, numeric_limits<double>::max()));
        repeatUpper = min(d, repeatUpper);
    }
    else repeatUpper = min(ub, repeatUpper);
}
void SymbolicDouble::clipRepeatUpperBound(const std::string& ub, bool closed)
{
    clipTRepeatUpperBound(stod(ub), closed);
}

void SymbolicDouble::setTRepeatUpperBound(const double& ub, bool closed)
{
    if (!closed)
    {
        repeatUpper = (ub == numeric_limits<double>::max() ?
                       numeric_limits<double>::lowest() : nextafter(ub, numeric_limits<double>::max()));
    }
    else repeatUpper = ub;
}
void SymbolicDouble::setRepeatUpperBound(const std::string& ub, bool closed)
{
    setTRepeatUpperBound(stod(ub), closed);
}

void SymbolicDouble::iterateTo(double toD, bool closed)
{
    if (toD < lowerBound)
    {
        if (minChange > 0) throw std::runtime_error("cant move downwards");
        if (numeric_limits<double>::lowest() - minChange > toD)
        {
            lowerBound = numeric_limits<double>::lowest();
        }
        else lowerBound = toD + minChange;
    }
    else if (toD > upperBound)
    {
        if (maxChange < 0) throw std::runtime_error("cant move downwards");
        if (numeric_limits<double>::max() - maxChange < toD)
        {
            upperBound = numeric_limits<double>::max();
        }
        else upperBound = toD + maxChange;
    }
}
void SymbolicDouble::iterateTo(const std::string& to, bool closed)
{
    double toD;
    try {toD = stod(to);}
    catch (invalid_argument&) {throw std::runtime_error("double asked to iterate to something else");}
    iterateTo(toD, closed);
}
void SymbolicDouble::iterateTo(SymbolicVariable* to, bool closed)
{
    SymbolicDouble* sd = static_cast<SymbolicDouble*>(to);
    if (upperBound <= sd->getTLowerBound()) iterateTo(sd->getTLowerBound(), closed);
    else if (lowerBound >= sd->getTUpperBound()) iterateTo(sd->getTUpperBound(), closed);
    else throw runtime_error("should have been one of those");
}

bool SymbolicDouble::getRelativeVelocity(SymbolicVariable* other, long double& slowest, long double& fastest) const
{
    if (other->getType() != DOUBLE) throw runtime_error("should be double");
    SymbolicDouble* sd = static_cast<SymbolicDouble*>(other);
    fastest = maxChange - sd->minChange;
    slowest = minChange - sd->maxChange;
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

SymbolicVariable::MonotoneEnum SymbolicDouble::getMonotonicity() const
{
    if (!uniformlyChanging) return NONE;
    else if (minChange > 0) return INCREASING;
    else if (maxChange < 0) return DECREASING;
    else if (minChange == 0 &&  maxChange == 0) return FRESH;
    else return UNKNOWN;
}

void SymbolicDouble::addConstToLower(const double diff)
{
    if (!defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

    if (!isBoundedBelow())
    {
        if (diff < 0) reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly drop below limit");
        else lowerBound = numeric_limits<double>::lowest() + diff; //kind of assuming diff is not too big
    }
    else
    {
        if (diff > 0)
        {
            if (lowerBound > numeric_limits<double>::max() - diff) reportError(Reporter::AlertType::RANGE, varN + " will overflow");
            else lowerBound += diff;
        }
        else
        {
            if (lowerBound < numeric_limits<double>::lowest() - diff)
            {
                reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly exceed double limits");
            }
            else lowerBound += diff;
        }
    }
}

void SymbolicDouble::addConstToUpper(const double diff)
{
    if (!defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

    if (!isBoundedAbove())
    {
        if (diff > 0) reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly exceed double limits");
        else upperBound = numeric_limits<double>::max() + diff;
    }
    else
    {
        if (diff > 0)
        {
            if (upperBound > numeric_limits<double>::max() - diff)
            {
                reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly exceed double limits");
                upperBound = numeric_limits<double>::max();
            }
            else upperBound += diff;
        }
        else
        {
            if (upperBound < numeric_limits<double>::lowest() - diff) reportError(Reporter::AlertType::RANGE, varN + " will overflow");
            else upperBound += diff;
        }
    }
}

void SymbolicDouble::addConst(double diff)
{
    if (!defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

    if (diff == 0)
    {
        reporter.warn(Reporter::AlertType::USELESS_OP, "Zero added to " + varN);
        return;
    }

    if (!isDetermined())
    {
        addConstToLower(diff);
        addConstToUpper(diff);
    }

    else
    {
        double oldT = lowerBound;
        if (diff > 0 && oldT > numeric_limits<double>::max() - diff) reportError(Reporter::AlertType::RANGE, varN + " will overflow");
        else if (diff < 0 && oldT < numeric_limits<double>::lowest() - diff)  reportError(Reporter::AlertType::RANGE, varN + " will overflow");
        upperBound = lowerBound = oldT + diff;
    }

    minChange += diff;
    maxChange += diff;
}

void SymbolicDouble::addSymbolicDouble(SymbolicDouble& other, bool increment)
{
    if (!other.defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, other.varN + " used before explicitly initialised");

    if (other.isDetermined())
    {
        addConst(other.getTConstValue());
        return;
    }

    if (!defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

    double otherLowerBound = other.getTLowerBound();
    double otherUpperBound = other.getTUpperBound();

    addConstToLower(otherLowerBound);
    addConstToUpper(otherUpperBound);

    if (!increment)
    {
        minChange += other.minChange;
        maxChange += other.maxChange;
    }
    else
    {
        minChange += other.getTLowerBound();
        maxChange += other.getTUpperBound();
    }
}

void SymbolicDouble::minusSymbolicDouble(SymbolicDouble& other, bool increment)
{
    if (!other.defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, other.varN + " used before explicitly initialised");

    if (other.isDetermined())
    {
        addConst(other.getTConstValue());
        return;
    }

    if (!defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

    double otherLowerBound = other.getTLowerBound();
    double otherUpperBound = other.getTUpperBound();

    addConstToLower(-otherUpperBound);
    addConstToUpper(-otherLowerBound);

    if (increment)
    {
        minChange -= other.minChange;
        maxChange -= other.maxChange;
    }
    else
    {
        minChange -= other.getTLowerBound();
        maxChange -= other.getTUpperBound();
    }
}

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

void SymbolicDouble::multConst(double mul)
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

    if (!defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");
    if (mul == 0) setTConstValue(0);
    else if (mul == 1) reporter.warn(Reporter::AlertType::USELESS_OP, varN + "multiplied by 1");
    else
    {
        if (isDetermined())
        {
            double temp;
            ArithResult result = safeMultiply(getTConstValue(), mul, temp);
            if (result != FINE) reportError(Reporter::AlertType::RANGE, to_string(getTConstValue()) + " * " + to_string(mul) + " = overflow");
            else setTConstValue(temp);
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

            if (alwaysabove || alwaysbelow) reportError(Reporter::AlertType::RANGE, varN + " guaranteed to overflow when multiplied by " + to_string(mul));
            else if (bad) reporter.warn(Reporter::AlertType::RANGE, varN + " might overflow when multiplied by " + to_string(mul));

            if (lowerResult <= upperResult)
            {
                setTLowerBound(lowerResult);
                setTUpperBound(upperResult);
            }
            else
            {
                setTLowerBound(upperResult);
                setTUpperBound(lowerResult);
            }
        }
    }
}

void SymbolicDouble::multSymbolicDouble(SymbolicDouble &other)
{
    if (!other.defined)
    {
        reporter.warn(Reporter::AlertType::UNINITIALISED_USE, other.varN + " used before explicitly initialised");
    }
    if (other.isDetermined())
    {
        multConst(other.getTConstValue());
        return;
    }
    if (!defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

    double otherLowerBound = other.getTLowerBound();
    double otherUpperBound = other.getTUpperBound();

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
        reportError(Reporter::AlertType::RANGE, varN + " guaranteed to overflow when multiplied by " + other.varN);
    }
    else if (bad) reporter.warn(Reporter::AlertType::RANGE, varN + " might overflow when multiplied by " + other.varN);


    double oldLower = lowerBound;
    double oldUpper = upperBound;

    setTLowerBound(min(lowerlower, min(lowerupper, min(upperlower, upperupper))));
    setTUpperBound(max(lowerlower, max(lowerupper, max(upperlower, upperupper))));

    //todo make this more accurate
    minChange += upperBound - oldUpper;
    maxChange += lowerBound - oldLower;
}

void SymbolicDouble::modConst(double modulus)
{
    if (modulus == 0)
    {
        reportError(Reporter::AlertType::ZERODIVISION, varN + " divided by zero");
        return;
    }

    if (lowerBound < 0 || upperBound > modulus) uniformlyChanging = false;
    if (isDetermined())
    {
        setTConstValue(fmod(getTConstValue(),modulus));
        return;
    }
    else
    {
        setTLowerBound(0);
        modulus = abs(modulus);
        if (upperBound >= 0) setTUpperBound(min(upperBound, modulus));
        else setTUpperBound(modulus);
    }
}

void SymbolicDouble::modSymbolicDouble(SymbolicDouble &other)
{
    if (other.isDetermined())
    {
        double otherVal = other.getTConstValue();
        if (otherVal == 0)
        {
            reportError(Reporter::ZERODIVISION, varN + " divided by " + other.varN + " ( = 0)");
            return;
        }
        else modConst(otherVal);
    }

    else
    {
        if (lowerBound < 0 || upperBound > other.lowerBound) uniformlyChanging = false;
        setTLowerBound(0);
        if (other.lowerBound <= 0 && other.upperBound >= 0)
        {
            reporter.warn(Reporter::ZERODIVISION,  varN + " divided by " + other.varN + " which could possibly be zero");
        }
        setTUpperBound(max(upperBound, max(abs(other.lowerBound), abs(other.upperBound))));
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

void SymbolicDouble::divConst(double denom)
{
    if (!defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

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

    if (denom == 0) reportError(Reporter::AlertType::ZERODIVISION, varN + "divided by 0");
    else if (denom == 1) reporter.warn(Reporter::AlertType::USELESS_OP, varN + "divided by 1");
    else if (isDetermined())
    {
        double temp;
        ArithResult result = safeDivide(getTConstValue(), denom, temp);
        if (result != FINE) reportError(Reporter::AlertType::RANGE, to_string(getTConstValue()) + " / " + to_string(denom) + " = overflow");
        else setTConstValue(temp);
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

        if (alwaysabove || alwaysbelow) reportError(Reporter::AlertType::RANGE, varN + " guaranteed to overflow when divided by " + to_string(denom));
        else if (bad) reporter.warn(Reporter::AlertType::RANGE, varN + " might overflow when divided by " + to_string(denom));

        if (lowerResult <= upperResult)
        {
            setTLowerBound(lowerResult);
            setTUpperBound(upperResult);
        }
        else
        {
            setTLowerBound(upperResult);
            setTUpperBound(lowerResult);
        }

    }
}

void SymbolicDouble::divSymbolicDouble(SymbolicDouble &other)
{
    if (!other.defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, other.varN + " used before explicitly initialised");
    if (other.isDetermined())
    {
        divConst(other.getTConstValue());
        return;
    }
    if (!defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

    double otherLowerBound = other.getTLowerBound();
    double otherUpperBound = other.getTUpperBound();

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

    if (alwaysabove || alwaysbelow) reportError(Reporter::AlertType::RANGE, varN + " guaranteed to overflow when divided by " + other.varN);
    else if (bad) reporter.warn(Reporter::AlertType::RANGE, varN + " might overflow when divided by " + other.varN);


    setTLowerBound(min(lowerlower, min(lowerupper, min(upperlower, upperupper))));
    setTUpperBound(max(lowerlower, max(lowerupper, max(upperlower, upperupper))));

    //todo make this more accurate
    minChange += upperBound - oldUpper;
    maxChange += lowerBound - oldLower;
}
