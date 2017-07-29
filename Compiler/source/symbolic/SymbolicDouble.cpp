#include <limits>
#include <algorithm>
#include <math.h>

#include "SymbolicDouble.h"

enum ArithResult{OVER, UNDER, FINE};

SymbolicDouble::SymbolicDouble(std::string name, Reporter& reporter):
    upperBound(0), //zero initialised
    lowerBound(0),
    minStep(0),
    reporter(reporter),
    isConst(true),
    isInitialised(false),
    feasable(true),
    varN(name),
    monotonicity(FRESH){}

SymbolicDouble::SymbolicDouble(SymbolicDouble& other):
    reporter(other.reporter)
{
    upperBound = other.getUpperBound();
    lowerBound = other.getLowerBound();
    monotonicity = other.getMonotinicity();
    isConst = other.isConst;
    varN = other.varN;
    isInitialised = other.isInitialised;
    feasable = other.isFeasable();
    minStep = other.minStep;
}
void SymbolicDouble::reportError(Reporter::AlertType type, std::string err)
{
    reporter.error(type, err);
    feasable = false;
}


void SymbolicDouble::userInput()
{
    upperBound = std::numeric_limits<double>::max();
    upperBound = std::numeric_limits<double>::lowest();
    isConst = false;
    isInitialised = true;
    monotonicity = FRESH;
}


bool SymbolicDouble::isBoundedAbove() const
{
    return upperBound != std::numeric_limits<double>::max();
}


bool SymbolicDouble::isBoundedBelow() const
{
    return upperBound != std::numeric_limits<double>::lowest();
}


bool SymbolicDouble::isFeasable()
{
    if (lowerBound > upperBound) feasable = false;
    return feasable;
}


const MonotoneEnum SymbolicDouble::getMonotinicity() const
{
    return monotonicity;
}


const double SymbolicDouble::getMininumStep() const
{
    return minStep;
}


const double SymbolicDouble::getUpperBound() const
{
    upperBound;
}


const double SymbolicDouble::getLowerBound() const
{
    return lowerBound;
}


const double SymbolicDouble::getConstValue() const
{
    if (!isConst) throw "Not constant";
    return lowerBound; //could be upper
}


void SymbolicDouble::setLowerBound(const double lower)
{
    //if (!isInitialised) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");
    lowerBound = lower;
    isConst = lowerBound == upperBound;
    monotonicity = UNKNOWN;
}


void SymbolicDouble::setUpperBound(const double upper)
{
    upperBound = upper;
    isConst = lowerBound == upperBound;
    monotonicity = UNKNOWN;
}


void SymbolicDouble::setConstValue(const double c)
{
    upperBound = lowerBound = c;
    isConst = true;
    isInitialised = true;
    monotonicity = FRESH;
}

bool SymbolicDouble::isDetermined() const
{
    return isConst;
}

void SymbolicDouble::addConstToLower(const double diff)
{
    if (!isInitialised) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

    if (!isBoundedBelow())
    {
        if (diff < 0)
        {
            reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly drop below limit");
        }
        else lowerBound = std::numeric_limits<double>::lowest() + diff; //kind of assuming diff is not too big
    }
    else
    {
        if (diff > 0)
        {
            if (lowerBound > std::numeric_limits<double>::max() - diff)
            {
                reportError(Reporter::AlertType::RANGE, varN + " will overflow");
            }
            else lowerBound += diff;
        }
        else
        {
            if (lowerBound < std::numeric_limits<double>::lowest() - diff)
            {
                reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly overflow");
            }
            else lowerBound += diff;
        }
    }
}

void SymbolicDouble::addConstToUpper(const double diff)
{
    if (!isInitialised) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

    if (!isBoundedAbove())
    {
        if (diff > 0)
        {
            reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly overflow");
        }
        else upperBound = std::numeric_limits<double>::max() + diff;
    }
    else
    {
        if (diff > 0)
        {
            if (upperBound > std::numeric_limits<double>::max() - diff)
            {
                reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly overflow");
                upperBound = std::numeric_limits<double>::max();
                //monotonicity = CONST;
            }
            else upperBound += diff;
        }
        else
        {
            if (upperBound < std::numeric_limits<double>::lowest() - diff)
            {
                reportError(Reporter::AlertType::RANGE, varN + " will overflow");
            }
            else upperBound += diff;
        }
    }
}

void SymbolicDouble::addConst(double diff)
{
    if (!isInitialised) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

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
        if (diff > 0 && oldT > std::numeric_limits<double>::max() - diff)
        {
            reportError(Reporter::AlertType::RANGE, varN + " will overflow");
        }
        else if (diff < 0 && oldT < std::numeric_limits<double>::lowest() - diff)
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
    if (!other.isInitialised) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, other.varN + " used before explicitly initialised");

    if (other.isDetermined())
    {
        addConst(other.getConstValue());
        return;
    }

    if (!isInitialised) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

    double otherLowerBound = other.getLowerBound();
    double otherUpperBound = other.getUpperBound();

    addConstToLower(otherLowerBound);
    addConstToUpper(otherUpperBound);
}

ArithResult safeMultiply(double a, double b, double& result)
{
    if (a > std::numeric_limits<double>::max() / b)
    {
        result = std::numeric_limits<double>::max();
        return ArithResult::OVER;
    }
    else if (a < std::numeric_limits<double>::min() / b)
    {
        result = std::numeric_limits<double>::min();
        return ArithResult ::UNDER;
    }
    result = a * b;
    return ArithResult::FINE;
}

void SymbolicDouble::multConst(double mul)
{
    if (!isInitialised) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

    if (mul == 0) setConstValue(0);
    else if (mul == 1) reporter.warn(Reporter::AlertType::USELESS_OP, varN + "multiplied by 1");
    else if (isDetermined())
    {
        double temp;
        ArithResult result = safeMultiply(getConstValue(), mul, temp);
        if (result != FINE) reportError(Reporter::AlertType::RANGE, std::to_string(getConstValue()) + " * " + std::to_string(mul) + " = overflow");
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

        if (alwaysabove || alwaysbelow) reportError(Reporter::AlertType::RANGE, varN + " guaranteed to overflow when multiplied by " + std::to_string(mul));
        else if (bad) reporter.warn(Reporter::AlertType::RANGE, varN + " might overflow when multiplied by " + std::to_string(mul));

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
        setUpperBound(-temp);
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
            if (value > std::numeric_limits<double>::max()/mul) //will be >0 also
            {
                reportError(Reporter::AlertType ::RANGE, varN + " will overflow");
            }
            else if (value < std::numeric_limits<double>::lowest() / mul)
            {
                reportError(Reporter::AlertType ::RANGE, varN + " will overflow");
            }
            else setConstValue(value * mul);
        }
        else setConstValue(value * mul);
    }
    else
    {
        if (mul > 1)
        {
            if (!isBoundedAbove() || upperBound > std::numeric_limits<double>::max() / mul)
            {
                reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly overflow");
                upperBound = std::numeric_limits<double>::max();
            }
            else upperBound *= mul;

            if (!isBoundedBelow() || lowerBound < std::numeric_limits<double>::lowest() / mul)
            {
                reporter.warn(Reporter::AlertType::RANGE, varN + " could possibly overflow");
                lowerBound = std::numeric_limits<double>::lowest();
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
    if (!other.isInitialised) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, other.varN + " used before explicitly initialised");
    if (other.isDetermined())
    {
        multConst(other.getConstValue());
        return;
    }
    if (!isInitialised) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

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


    setLowerBound(std::min(lowerlower, std::min(lowerupper, std::min(upperlower, upperupper))));
    setUpperBound(std::max(lowerlower, std::max(lowerupper, std::max(upperlower, upperupper))));

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
        setUpperBound(temp);

        temp = -otherLowerBound;
        otherLowerBound = -otherUpperBound;
        otherUpperBound = temp;
    }

    //can now assume otherUpperBound >= 0, otherLowerBound =< 0, and |otherUpperBound| >= |otherLowerBound|
    if (otherUpperBound < 0 || otherLowerBound > 0 || abs(otherLowerBound) < abs(otherUpperBound))
        throw "not true"; //debug

    int myside = 0; //-1 if range is left of zero
    if (upperBound < 0) myside = -1;
    else if (lowerBound > 0) myside = 1;

    int theirside = 0; //-1 if their range is left of *1*
    if (otherUpperBound < 1) theirside = -1;
    else if (otherLowerBound > 1) theirside = 1;

    

    if (upperBound > std::numeric_limits<double>::max() / otherUpperBound)
    {
        upperBound = std::numeric_limits<double>::max();//warning
    }
    else upperBound *= otherUpperBound;

    if (sameSign(lowerBound, otherLowerBound))
    {
        if (lowerBound < std::numeric_limits<double>::max() / otherLowerBound)
        {
            reportError(Reporter::AlertType ::RANGE, varN + " will overflow");
        }
        else lowerBound *= otherLowerBound;
    }
    else
    {
        if (lowerBound < std::numeric_limits<double>::min() / otherUpperBound)
        {
            lowerBound = std::numeric_limits<double>::min();
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
        setConstValue(fmod(getConstValue(),modulus));
        return;
    }
    else
    {
        setLowerBound(0);
        if (upperBound >= 0) setUpperBound(std::min(upperBound, modulus));
        else setUpperBound(modulus);
    }
}

void SymbolicDouble::modSymbolicDouble(SymbolicDouble &other)
{
    if (other.isDetermined())
    {
        double otherVal = other.getConstValue();
        if (otherVal == 0)
        {
            reportError(Reporter::AlertType::ZERODIVISION, varN + " divided by " + other.varN + " ( = 0)");
            return;
        }
        else modConst(otherVal);
    }

    else
    {
        setLowerBound(0);
        setUpperBound(std::max(abs(other.lowerBound), abs(other.upperBound)));
    }
}

ArithResult safeDivide(double a, double b, double& result)
{
    if (b >= 1)
    {
        result = a / b;
        return ArithResult::FINE;
    }
    if (a > std::numeric_limits<double>::max() * b)
    {
        result = std::numeric_limits<double>::max();
        return ArithResult::OVER;
    }
    else if (a < std::numeric_limits<double>::min() * b)
    {
        result = std::numeric_limits<double>::min();
        return ArithResult ::UNDER;
    }
    result = a / b;
    return ArithResult::FINE;
}

void SymbolicDouble::divConst(double denom)
{
    if (!isInitialised) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

    if (denom == 0) reportError(Reporter::AlertType::ZERODIVISION, varN + "divided by 0");
    else if (denom == 1) reporter.warn(Reporter::AlertType::USELESS_OP, varN + "divided by 1");
    else if (isDetermined())
    {
        double temp;
        ArithResult result = safeDivide(getConstValue(), denom, temp);
        if (result != FINE) reportError(Reporter::AlertType::RANGE, std::to_string(getConstValue()) + " / " + std::to_string(denom) + " = overflow");
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

        if (alwaysabove || alwaysbelow) reportError(Reporter::AlertType::RANGE, varN + " guaranteed to overflow when divided by " + std::to_string(denom));
        else if (bad) reporter.warn(Reporter::AlertType::RANGE, varN + " might overflow when divided by " + std::to_string(denom));

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
    if (!other.isInitialised) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, other.varN + " used before explicitly initialised");
    if (other.isDetermined())
    {
        divConst(other.getConstValue());
        return;
    }
    if (!isInitialised) reporter.warn(Reporter::AlertType::UNINITIALISED_USE, varN + " used before explicitly initialised");

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


    setLowerBound(std::min(lowerlower, std::min(lowerupper, std::min(upperlower, upperupper))));
    setUpperBound(std::max(lowerlower, std::max(lowerupper, std::max(upperlower, upperupper))));

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
