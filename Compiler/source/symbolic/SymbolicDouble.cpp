//
// Created by abdul on 14/08/17.
//
#include <limits>
#include <cmath>
#include "SymbolicVariables.h"

using namespace std;

enum ArithResult{OVER, UNDER, FINE};

SymbolicDouble::SymbolicDouble(string name, Reporter& r):
        SymbolicVariableTemplate(move(name), 0, 0, r, DOUBLE)
{}

SymbolicDouble::SymbolicDouble(SymbolicDouble* other):
        SymbolicVariableTemplate
                (other->getName(), other->getTLowerBound(), other->getTUpperBound(), other->reporter, DOUBLE, other->defined),
                minChange(other->minChange), maxChange(other->maxChange), uniformlyChanging(other->uniformlyChanging)
{}

SymbolicDouble::SymbolicDouble(SymbolicVariable* other):
    SymbolicDouble(static_cast<SymbolicDouble*>(other)) {}//watch out!

unique_ptr<SymbolicVariable> SymbolicDouble::clone()
{
    return make_unique<SymbolicDouble>(*this);
}

void SymbolicDouble::userInput()
{
    upperBound = numeric_limits<double>::max();
    lowerBound = numeric_limits<double>::lowest();
    define();
    uniformlyChanging = false;
}

void SymbolicDouble::loopInit()
{
    minChange = maxChange = 0;
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
        case Relations::NE:
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
            throw "weird enum";
    }
}

SymbolicVariable::MeetEnum SymbolicDouble::canMeet(Relations::Relop rel, const std::string& rhstring) const
{
    double rhs = stod(rhstring);
    switch(rel)
    {
        case Relations::EQ:
            if (isDetermined() && getTLowerBound() == rhs) return MUST;
            else if (rhs >= getTLowerBound() && rhs <= getTLowerBound()) return MAY;
            else return CANT;
        case Relations::NE:
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
    if (!closed && d != numeric_limits<double>::max()) upperBound = nextafter(d, numeric_limits<double>::max());
    else upperBound = d;

    if (lowerBound > upperBound) feasable = false;
    return isFeasable();
}
bool SymbolicDouble::setUpperBound(const std::string& ub, bool closed)
{
    return setTUpperBound(stod(ub), closed);
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
    if (d < getTLowerBound()) return setTLowerBound(d, closed);
    else return isFeasable();
}
bool SymbolicDouble::unionUpperBound(const std::string& ub, bool closed)
{
    return unionTLowerBound(stod(ub), closed);
}

bool SymbolicDouble::unionTUpperBound(const double& d, bool closed)
{
    if (d > getTUpperBound()) return setTUpperBound(d, closed);
    else return isFeasable();
}
bool SymbolicDouble::unionLowerBound(const std::string& lb, bool closed)
{
    return unionTLowerBound(stod(lb), closed);
}

void SymbolicDouble::setTConstValue(const double& d)
{
    SymbolicVariableTemplate<double>::setTConstValue(d);
    uniformlyChanging = false;
}
void SymbolicDouble::setConstValue(const std::string& c)
{
    setTConstValue(stod(c));
}

void SymbolicDouble::iterateTo(const std::string& to, bool closed)
{
    double toD;
    try {toD = stod(to);}
    catch (invalid_argument&) {throw "double asked to iterate to something else";}

    if (toD < lowerBound)
    {
        if (minChange > 0) throw "cant move downwards";
        if (numeric_limits<double>::lowest() - minChange > toD)
        {
            lowerBound = numeric_limits<double>::lowest();
        }
        else lowerBound = toD + minChange;

        //upperBound = closed ? toD : nextafter(toD, numeric_limits<double>::lowest());
    }
    else if (toD > upperBound)
    {
        if (maxChange < 0) throw "cant move downwards";
        if (numeric_limits<double>::max() - maxChange < toD)
        {
            upperBound = numeric_limits<double>::max();
        }
        else upperBound = toD + maxChange;

        //lowerBound = closed ? toD : nextafter(toD, numeric_limits<double>::max());
    }
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

void SymbolicDouble::addSymbolicDouble(SymbolicDouble& other)
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

    minChange += otherLowerBound;
    maxChange += otherUpperBound;
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
    double oldUpper = upperBound;
    double oldLower = lowerBound;

    if (!defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");
    if (mul == 0) setConstValue(0);
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

            ArithResult result = safeMultiply(oldLower, mul, lowerResult);
            if (result == FINE) alwaysabove = alwaysbelow = false;
            else
            {
                bad = true;
                if (result == OVER) alwaysbelow = false;
                else alwaysabove = false;
            }

            result = safeMultiply(oldUpper, mul, upperResult);
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
    if (mul >= 0)
    {
        minChange += lowerBound - oldLower;
        maxChange += upperBound - oldUpper;
    }
    else
    {
        minChange += lowerBound - oldUpper;
        maxChange += upperBound - oldLower;
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
