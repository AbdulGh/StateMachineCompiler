//
// Created by abdul on 14/08/17.
//
#include <limits>
#include <math.h>
#include "SymbolicVariables.h"

using namespace std;

enum ArithResult{OVER, UNDER, FINE};

SymbolicDouble::SymbolicDouble(string name, Reporter& r):
        SymbolicVariableTemplate(move(name), 0, 0, r, DOUBLE), minStep(0), monotonicity(FRESH)
{}

SymbolicDouble::SymbolicDouble(SymbolicDouble& other):
        SymbolicVariableTemplate(other.getName(), other.getLowerBound(), other.getUpperBound(), other.reporter, DOUBLE, other.defined)
{
    monotonicity = other.getMonotonicity();
    isConst = other.isConst;
    feasable = other.isFeasable();
    minStep = other.minStep;
}

SymbolicDouble::SymbolicDouble(shared_ptr<SymbolicDouble> other): SymbolicDouble(*other.get()) {}

SymbolicDouble::SymbolicDouble(shared_ptr<SymbolicVariable> other):
        SymbolicDouble(static_pointer_cast<SymbolicDouble>(other)) {} //look out!

shared_ptr<SymbolicVariable> SymbolicDouble::clone()
{
    return make_shared<SymbolicDouble>(this);
}

void SymbolicDouble::userInput()
{
    upperBound = numeric_limits<double>::max();
    lowerBound = numeric_limits<double>::lowest();
    isConst = false;
    define();
    monotonicity = NONE;
}

SymbolicVariable::MeetEnum SymbolicDouble::canMeet(Relations::Relop rel, const std::string& rhstring) const
{
    double rhs = stod(rhstring);
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

bool SymbolicDouble::setTLowerBound(const double& d, bool closed)
{
    if (!closed && d != numeric_limits<double>::lowest()) lowerBound = d - numeric_limits<double>::min();
    else lowerBound = d;

    if (lowerBound > upperBound) feasable = false;
    else if (lowerBound == upperBound) isConst = true;
    return isFeasable();
}
bool SymbolicDouble::setLowerBound(const std::string& lb, bool closed)
{
    setTLowerBound(stod(lb), closed);
}

bool SymbolicDouble::setTUpperBound(const double& d, bool closed)
{
    if (!closed && d != numeric_limits<double>::max()) upperBound = d + numeric_limits<double>::min();
    else upperBound = d;

    if (lowerBound > upperBound) feasable = false;
    else if (lowerBound == upperBound) isConst = true;
    return isFeasable();
}
bool SymbolicDouble::setUpperBound(const std::string& ub, bool closed)
{
    return setTUpperBound(stod(ub), closed);
}

bool SymbolicDouble::clipTLowerBound(const double& d, bool closed)
{
    if (d > getLowerBound()) return setLowerBound(d, closed);
    else return isFeasable();
}
bool SymbolicDouble::clipUpperBound(const std::string& ub, bool closed)
{
    return clipTUpperBound(stod(ub), closed);
}

bool SymbolicDouble::clipTUpperBound(const double& d, bool closed)
{
    if (d < getUpperBound()) return setUpperBound(d, closed);
    else return isFeasable();
}
bool SymbolicDouble::clipLowerBound(const std::string& lb, bool closed)
{
    return clipTLowerBound(stod(lb), closed);
}

bool SymbolicDouble::unionTLowerBound(const double& d, bool closed)
{
    if (d < getLowerBound()) return setLowerBound(d, closed);
    else return isFeasable();
}
bool SymbolicDouble::unionUpperBound(const std::string& ub, bool closed)
{
    return unionTLowerBound(stod(ub), closed);
}

bool SymbolicDouble::unionTUpperBound(const double& d, bool closed)
{
    if (d > getUpperBound()) return setUpperBound(d, closed);
    else return isFeasable();
}
bool SymbolicDouble::unionLowerBound(const std::string& lb, bool closed)
{
    return unionTLowerBound(stod(lb), closed);
}

void SymbolicDouble::setConstValue(const std::string& c)
{
    setTConstValue(stod(c));
}

bool SymbolicDouble::isBoundedAbove() const
{
    return upperBound != numeric_limits<double>::max();
}

bool SymbolicDouble::isBoundedBelow() const
{
    return upperBound != numeric_limits<double>::lowest();
}

void SymbolicDouble::addConstToLower(const double diff)
{
    if (!defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

    if (!isBoundedBelow())
    {
        if (diff < 0)
        {
            reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly drop below limit");
        }
        else lowerBound = numeric_limits<double>::lowest() + diff; //kind of assuming diff is not too big
    }
    else
    {
        if (diff > 0)
        {
            if (lowerBound > numeric_limits<double>::max() - diff)
            {
                reportError(Reporter::AlertType::RANGE, varN + " will overflow");
            }
            else lowerBound += diff;
        }
        else
        {
            if (lowerBound < numeric_limits<double>::lowest() - diff)
            {
                reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly overflow");
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
        if (diff > 0)
        {
            reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly overflow");
        }
        else upperBound = numeric_limits<double>::max() + diff;
    }
    else
    {
        if (diff > 0)
        {
            if (upperBound > numeric_limits<double>::max() - diff)
            {
                reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly overflow");
                upperBound = numeric_limits<double>::max();
                //monotonicity = CONST;
            }
            else upperBound += diff;
        }
        else
        {
            if (upperBound < numeric_limits<double>::lowest() - diff)
            {
                reportError(Reporter::AlertType::RANGE, varN + " will overflow");
            }
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
        if (diff > 0 && oldT > numeric_limits<double>::max() - diff)
        {
            reportError(Reporter::AlertType::RANGE, varN + " will overflow");
        }
        else if (diff < 0 && oldT < numeric_limits<double>::lowest() - diff)
        {
            reportError(Reporter::AlertType::RANGE, varN + " will overflow");
        }
        upperBound = lowerBound = oldT + diff;
    }

    if (monotonicity == FRESH)
    {
        monotonicity = (diff > 0) ? INCREASING : DECREASING;
        minStep = diff;
    }
    else if (monotonicity == INCREASING)
    {
        if (diff < 0)
        {
            monotonicity = NONE;
            minStep = 0;
        }
        else if (diff < minStep)  minStep = diff;
    }
    else if (monotonicity == DECREASING)
    {
        if (diff > 0)
        {
            monotonicity = NONE;
            minStep = 0;
        }
        else if (diff > minStep)  minStep = diff;
    }
}
void SymbolicDouble::addSymbolicDouble(SymbolicDouble& other)
{
    if (!other.defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, other.varN + " used before explicitly initialised");

    if (other.isDetermined())
    {
        addConst(other.getConstValue());
        return;
    }

    if (!defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

    double otherLowerBound = other.getLowerBound();
    double otherUpperBound = other.getUpperBound();

    addConstToLower(otherLowerBound);
    addConstToUpper(otherUpperBound);
}

ArithResult safeMultiply(double a, double b, double& result)
{
    if (a > numeric_limits<double>::max() / b)
    {
        result = numeric_limits<double>::max();
        return ArithResult::OVER;
    }
    else if (a < numeric_limits<double>::min() / b)
    {
        result = numeric_limits<double>::min();
        return ArithResult::UNDER;
    }
    result = a * b;
    return ArithResult::FINE;
}

void SymbolicDouble::multConst(double mul)
{
    if (!defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

    if (mul == 0) setConstValue(0);
    else if (mul == 1) reporter.warn(Reporter::AlertType::USELESS_OP, varN + "multiplied by 1");
    else if (isDetermined())
    {
        double temp;
        ArithResult result = safeMultiply(getConstValue(), mul, temp);
        if (result != FINE) reportError(Reporter::AlertType::RANGE, to_string(getConstValue()) + " * " + to_string(mul) + " = overflow");
        else
        {
            double oldconst = getConstValue();
            if (monotonicity == FRESH)
            {
                if (oldconst < temp) monotonicity = INCREASING;
                else if (oldconst > temp) monotonicity = DECREASING;
                else monotonicity = NONE;
            }
            else if (monotonicity == INCREASING && oldconst > temp || monotonicity == DECREASING && oldconst < temp) monotonicity = NONE;
            setConstValue(temp);
        }
    }
    else
    {
        double oldupper = upperBound;
        double oldlower = lowerBound;
        double lowerResult, upperResult;

        bool bad = false;
        bool alwaysabove = true;
        bool alwaysbelow = true;

        ArithResult result = safeMultiply(oldlower, mul, lowerResult);
        if (result == FINE) alwaysabove = alwaysbelow = false;
        else
        {
            bad = true;
            if (result == OVER) alwaysbelow = false;
            else alwaysabove = false;
        }

        result = safeMultiply(oldupper, mul, upperResult);
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

        if (upperBound > oldupper && lowerBound > oldlower)
        {
            if (monotonicity == FRESH) monotonicity = INCREASING;
            else if (monotonicity == DECREASING) monotonicity = NONE;
        }
        else if (oldupper < oldupper && lowerBound < oldlower)
        {
            if (monotonicity == FRESH) monotonicity = DECREASING;
            else if (monotonicity == DECREASING) monotonicity = NONE;
        }
        else monotonicity = NONE;
    }
    /*
    if (mul < 0) //flip upper/lower
    {
        double temp = lowerBound;
        setLowerBound(-upperBound);
        setTUpperBound(-temp);
        mul *= -1;
        monotonicity = NONE;
    }

    if (mul == 1)
    {
        return;
    }
    //can assume now mul is > 0 and != 1

    int direction = 0; //-1 if range is shifting down, 1 if shifting up, 0 otherwise
    if (upperBound <= 0) direction = (mul > 1) ? -1 : 1;
    else if (lowerBound >= 0) direction = (mul > 1) ? 1 : -1;

    if (direction != 0)
    {
        if (monotonicity == FRESH) monotonicity = (direction == 1) ? INCREASING : DECREASING;
        else if (monotonicity == INCREASING && direction == -1 || monotonicity == DECREASING && direction == 1) monotonicity = NONE;
    }

    if (isDetermined())
    {
        double value = getConstValue();

        if (mul > 1)
        {
            if (value > numeric_limits<double>::max()/mul) //will be >0 also
            {
                reportError(Reporter::AlertType::RANGE, varN + " will overflow");
            }
            else if (value < numeric_limits<double>::lowest() / mul)
            {
                reportError(Reporter::AlertType::RANGE, varN + " will overflow");
            }
            else setConstValue(value * mul);
        }
        else setConstValue(value * mul);
    }
    else
    {
        if (mul > 1)
        {
            if (!isBoundedAbove() || upperBound > numeric_limits<double>::max() / mul)
            {
                reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly overflow");
                upperBound = numeric_limits<double>::max();
            }
            else upperBound *= mul;

            if (!isBoundedBelow() || lowerBound < numeric_limits<double>::lowest() / mul)
            {
                reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly overflow");
                lowerBound = numeric_limits<double>::lowest();
            }
            else lowerBound *= mul;
        }

        else
        {
            lowerBound *= mul;
            upperBound *= mul;
        }
    }*/
}

void SymbolicDouble::multSymbolicDouble(SymbolicDouble &other)
{
    if (!other.defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, other.varN + " used before explicitly initialised");
    if (other.isDetermined())
    {
        multConst(other.getConstValue());
        return;
    }
    if (!defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

    double otherLowerBound = other.getLowerBound();
    double otherUpperBound = other.getUpperBound();

    double oldupper = upperBound;
    double oldlower = lowerBound;


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

    if (alwaysabove || alwaysbelow) reportError(Reporter::AlertType::RANGE, varN + " guaranteed to overflow when multiplied by " + other.varN);
    else if (bad) reporter.warn(Reporter::AlertType::RANGE, varN + " might overflow when multiplied by " + other.varN);


    setTLowerBound(min(lowerlower, min(lowerupper, min(upperlower, upperupper))));
    setTUpperBound(max(lowerlower, max(lowerupper, max(upperlower, upperupper))));

    if (upperBound > oldupper && lowerBound > oldlower)
    {
        if (monotonicity == FRESH) monotonicity = INCREASING;
        else if (monotonicity == DECREASING) monotonicity = NONE;
    }

    else if (upperlower < oldupper && lowerBound < oldlower)
    {
        if (monotonicity == FRESH) monotonicity = DECREASING;
        else if (monotonicity == DECREASING) monotonicity = NONE;
    }

    else monotonicity = NONE;

    /* todo make the above less brute-forcey
    if (abs(otherLowerBound) >= abs(otherUpperBound)) //multiply by (-1)^2. Includes case when otherUpperBound < 0
    {
        double temp = -lowerBound;
        setLowerBound(-upperBound);
        setTUpperBound(temp);

        temp = -otherLowerBound;
        otherLowerBound = -otherUpperBound;
        otherUpperBound = temp;
    }

    //can now assume otherUpperBound >= 0, otherLowerBound =< 0, and |otherUpperBound| >= |otherLowerBound|
    if (otherUpperBound < 0 || otherLowerBound > 0 || abs(otherLowerBound) < abs(otherUpperBound))
        throw "not true";

    int myside = 0; //-1 if range is left of zero
    if (upperBound < 0) myside = -1;
    else if (lowerBound > 0) myside = 1;

    int theirside = 0; //-1 if their range is left of *1*
    if (otherUpperBound < 1) theirside = -1;
    else if (otherLowerBound > 1) theirside = 1;



    if (upperBound > numeric_limits<double>::max() / otherUpperBound)
    {
        upperBound = numeric_limits<double>::max();//warning
    }
    else upperBound *= otherUpperBound;

    if (sameSign(lowerBound, otherLowerBound))
    {
        if (lowerBound < numeric_limits<double>::max() / otherLowerBound)
        {
            reportError(Reporter::AlertType::RANGE, varN + " will overflow");
        }
        else lowerBound *= otherLowerBound;
    }
    else
    {
        if (lowerBound < numeric_limits<double>::min() / otherUpperBound)
        {
            lowerBound = numeric_limits<double>::min();
        }
        else lowerBound *= otherUpperBound;
    }
     */
}

void SymbolicDouble::modConst(double modulus)
{
    if (modulus == 0)
    {
        reportError(Reporter::AlertType::ZERODIVISION, varN + " divided by zero");
        return;
    }
    if (isDetermined())
    {
        setTConstValue(fmod(getConstValue(),modulus));
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
        double otherVal = other.getConstValue();
        if (otherVal == 0)
        {
            reportError(Reporter::ZERODIVISION, varN + " divided by " + other.varN + " ( = 0)");
            return;
        }
        else modConst(otherVal);
    }

    else
    {
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
    if (b >= 1)
    {
        result = a / b;
        return ArithResult::FINE;
    }
    if (a > numeric_limits<double>::max() * b)
    {
        result = numeric_limits<double>::max();
        return ArithResult::OVER;
    }
    else if (a < numeric_limits<double>::min() * b)
    {
        result = numeric_limits<double>::min();
        return ArithResult::UNDER;
    }
    result = a / b;
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
        ArithResult result = safeDivide(getConstValue(), denom, temp);
        if (result != FINE) reportError(Reporter::AlertType::RANGE, to_string(getConstValue()) + " / " + to_string(denom) + " = overflow");
        else
        {
            double oldconst = getConstValue();
            if (monotonicity == FRESH)
            {
                if (oldconst < temp) monotonicity = INCREASING;
                else if (oldconst > temp) monotonicity = DECREASING;
                else monotonicity = NONE;
            }
            else if (monotonicity == INCREASING && oldconst > temp || monotonicity == DECREASING && oldconst < temp) monotonicity = NONE;
            setTConstValue(temp);
        }
    }
    else
    {
        double oldupper = upperBound;
        double oldlower = lowerBound;
        double lowerResult, upperResult;

        bool bad = false;
        bool alwaysabove = true;
        bool alwaysbelow = true;

        ArithResult result = safeDivide(oldlower, denom, lowerResult);
        if (result == FINE) alwaysabove = alwaysbelow = false;
        else
        {
            bad = true;
            if (result == OVER) alwaysbelow = false;
            else alwaysabove = false;
        }

        result = safeDivide(oldupper, denom, upperResult);
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

        if (upperBound > oldupper && lowerBound > oldlower)
        {
            if (monotonicity == FRESH) monotonicity = INCREASING;
            else if (monotonicity == DECREASING) monotonicity = NONE;
        }
        else if (oldupper < oldupper && lowerBound < oldlower)
        {
            if (monotonicity == FRESH) monotonicity = DECREASING;
            else if (monotonicity == DECREASING) monotonicity = NONE;
        }
        else monotonicity = NONE;
    }
}

void SymbolicDouble::divSymbolicDouble(SymbolicDouble &other)
{
    if (!other.defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, other.varN + " used before explicitly initialised");
    if (other.isDetermined())
    {
        divConst(other.getConstValue());
        return;
    }
    if (!defined) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

    double otherLowerBound = other.getLowerBound();
    double otherUpperBound = other.getUpperBound();

    double oldupper = upperBound;
    double oldlower = lowerBound;


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

    if (upperBound > oldupper && lowerBound > oldlower)
    {
        if (monotonicity == FRESH) monotonicity = INCREASING;
        else if (monotonicity == DECREASING) monotonicity = NONE;
    }
    else if (oldupper < oldupper && lowerBound < oldlower)
    {
        if (monotonicity == FRESH) monotonicity = DECREASING;
        else if (monotonicity == DECREASING) monotonicity = NONE;
    }
    else monotonicity = NONE;
}
