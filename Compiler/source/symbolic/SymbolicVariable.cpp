#include <limits>

#include "SymbolicVariable.h"

template <typename T>
SymbolicVariable<T>::SymbolicVariable():
    upperBound(nullptr),
    lowerBound(nullptr),
    minStep(nullptr),
    monotonicity(FRESH){}

template <typename T>
void SymbolicVariable<T>::reset()
{
    if (upperBound != nullptr) delete upperBound;
    upperBound = nullptr;
    if (lowerBound != nullptr) delete lowerBound;
    lowerBound = nullptr;
    if (minStep != nullptr) delete minStep;
    minStep = nullptr;
    monotonicity = FRESH;
}

template <typename T>
SymbolicVariable<T>::SymbolicVariable(SymbolicVariable<T> &other)
{
    if (other.upperBound != nullptr) upperBound = new T(*other.upperBound);
    else upperBound = nullptr;

    if (other.lowerBound != nullptr) lowerBound = new T(*other.lowerBound);
    else lowerBound = nullptr;

    monotonicity = other.monotonicity;
}

template <typename T>
SymbolicVariable<T>::~SymbolicVariable()
{
    if (upperBound != nullptr) delete upperBound;
    if (lowerBound != nullptr) delete lowerBound;
    if (minStep != nullptr) delete minStep;
}

template <typename T>
bool SymbolicVariable<T>::isBoundedAbove() const
{
    return upperBound != nullptr;
}

template <typename T>
bool SymbolicVariable<T>::isBoundedBelow() const
{
    return upperBound != nullptr;
}

template <typename T>
bool SymbolicVariable<T>::isFeasable() const
{
    return *lowerBound <= *upperBound;
}

template <typename T>
const MonotoneEnum SymbolicVariable<T>::getMonotinicity() const
{
    return monotonicity;
}

template <typename T>
const T SymbolicVariable<T>::getMininumStep() const
{
    if (minStep == nullptr) throw "There is no minimum step!";
    return *minStep;
}

template <typename T>
const T SymbolicVariable<T>::getUpperBound() const
{
    if (upperBound == nullptr) throw "There is no upper bound!";
    return *upperBound;
}

template <typename T>
const T SymbolicVariable<T>::getLowerBound() const
{
    if (lowerBound == nullptr) throw "There is no lower bound!";
    return *lowerBound;
}

template <typename T>
const T SymbolicVariable<T>::getConstValue() const
{
    if (monotonicity != CONSTCONST && monotonicity != CONSTINCREASING && monotonicity != CONSTDECREASING) throw "Not constant";
    return *lowerBound; //could be upper
}

template <typename T>
void SymbolicVariable<T>::setLowerBound(const T& lower)
{
    if (isDetermined()) throw "Determined variables have no range";
    if (lowerBound != nullptr) delete lowerBound;
    lowerBound = new T(lower);
}

template <typename T>
void SymbolicVariable<T>::setUpperBound(const T& upper)
{
    if (isDetermined()) throw "Determined variables have no range";
    if (upperBound != nullptr) delete upperBound;
    upperBound = new T(upper);
}

template <typename T>
void SymbolicVariable<T>::setConstValue(const T& c)
{
    if (lowerBound != nullptr) delete lowerBound;
    if (upperBound != nullptr) delete upperBound;
    upperBound = lowerBound = new T(c); //I will be very careful
    monotonicity = CONSTCONST;
}

template <typename T>
bool SymbolicVariable<T>::isDetermined() const
{
    return (monotonicity == CONSTCONST || monotonicity == CONSTINCREASING
            || monotonicity == CONSTDECREASING || monotonicity == UNKNOWN);
}

template <>
void SymbolicVariable<double>::addConst(double diff)
{
    if (diff == 0)
    {
        //todo useless op
        return;
    }

    if (!isDetermined())
    {
        if (lowerBound == nullptr)
        {
            if (diff < 0)
            {
                //todo underflow warning
            }
            else lowerBound = new double(std::numeric_limits<double>::min() + diff); //kind of assuming diff is not too big
        }
        else
        {
            if (diff > 0)
            {
                if (*lowerBound > std::numeric_limits<double>::max() - diff)
                {
                    //todo overflow error and fail
                }
                else
                {
                    *lowerBound = *lowerBound + diff;
                }
            }
            else
            {
                if (*lowerBound < std::numeric_limits<double>::min() - diff)
                {
                    //todo underflow warning
                    delete lowerBound;
                }
                else
                {
                    *lowerBound = *lowerBound + diff;
                }
            }
        }

        if (upperBound == nullptr)
        {
            if (diff > 0)
            {
                //todo overflow warning
            }
            else upperBound = new double(std::numeric_limits<double>::max() + diff);
        }
        else
        {
            if (diff > 0)
            {
                if (*upperBound > std::numeric_limits<double>::max() - diff)
                {
                    //todo overflow warning
                    *upperBound = std::numeric_limits<double>::max();
                    //monotonicity = CONST;
                }
                else
                {
                    *upperBound = *upperBound + diff;
                }
            }
            else
            {
                if (*upperBound < std::numeric_limits<double>::min() - diff)
                {
                    //todo underflow warning and fail
                }
                else
                {
                    *upperBound = *upperBound + diff;
                }
            }
        }

    }

    else
    {
        if (monotonicity == CONSTCONST)
        {
            monotonicity = (diff > 0) ? CONSTINCREASING : CONSTDECREASING;
            if (minStep != nullptr)
            {
                throw "check";
            }
            minStep = new double(diff);
        }
        else if (monotonicity == CONSTINCREASING)
        {
            if (diff < 0) monotonicity = NONE;
            else
            {
                if (minStep == nullptr)
                {
                    throw "check";
                }
                else if (diff < *minStep) *minStep = diff;
            }
        }
        else if (monotonicity == CONSTDECREASING)
        {
            if (diff > 0) monotonicity = NONE;
            else if (diff > *minStep) *minStep = diff;
        }
    
        double oldT = *lowerBound;
        delete lowerBound;
        if (diff > 0 && oldT > std::numeric_limits<double>::max() - diff)
        {
            //todo fail w/ overflow
        }
        else if (diff < 0 && oldT < std::numeric_limits<double>::min() + diff)
        {
            //again
        }
        upperBound = lowerBound = new double(oldT + diff);
    }
}

template <>
void SymbolicVariable<double>::multConst(double mul) //todo think about how to incorporate bounds
{
    if (mul == 1)
    {
        return;
    }

    if (mul == 0)
    {
        monotonicity = CONSTCONST;
        if (lowerBound == nullptr) lowerBound = new double(0);
        else *lowerBound = 0;
        if (upperBound == nullptr) upperBound = new double(0);
        else *upperBound = 0;
        return;
    }

    if (mul < 0) //flip upper/lower
    {
        if (lowerBound == nullptr)
        {
            if (upperBound != nullptr)
            {
                lowerBound = new double(-(*upperBound));
                delete upperBound;
            }
        }
        else if (upperBound == nullptr)
        {
            //lowerBound is not null
            upperBound = new double(-(*lowerBound));
            delete lowerBound;
        }
        else
        {
            double temp = *upperBound;
            *upperBound = -(*lowerBound);
            *lowerBound = -temp;
        }

        delete minStep;
        monotonicity = NONE;
        mul *= -1;
    }

    //can assume now mul is > 0 and != 1

    if (isDetermined())
    {
        if (mul > 1)
        {
            //check upperbound
            if (upperBound == nullptr)
            {
                //todo overflow warning
            }
            else if (*upperBound > std::numeric_limits<double>::max() / mul)
            {
                //ditto
                delete upperBound;
            }
            else *upperBound = *upperBound * mul;

            //check lowerbound
            if (lowerBound == nullptr)
            {
                //todo overflow warning
            }
            else if (*lowerBound < std::numeric_limits<double>::min() / mul)
            {
                //ditto
                delete lowerBound;
            }
            else *lowerBound = *lowerBound * mul;
        }
        
        else
        {
            if (upperBound == nullptr) upperBound = new double(std::numeric_limits<double>::max());
            if (lowerBound == nullptr) lowerBound = new double(std::numeric_limits<double>::min());

            *upperBound = *upperBound * mul;
            *lowerBound = *lowerBound * mul;
        }
    }

    else
    {
        if (monotonicity == CONSTCONST)
        {
            if (mul > 1)
            {
                if (*upperBound > 1) monotonicity = CONSTINCREASING;
                else monotonicity = CONSTDECREASING;
            }
            else
            {
                if (*upperBound > 1) monotonicity = CONSTDECREASING;
                else monotonicity = CONSTINCREASING;
            }
        }
        else if (monotonicity == INCREASING)
        {
            if (mul < 1) monotonicity = NONE;
        }
        else if (monotonicity == DECREASING)
        {
            if (mul > 1) monotonicity = NONE;
        }

        double oldT = *lowerBound;
        delete lowerBound;
        if (mul > 1 && *upperBound > std::numeric_limits<double>::max() / mul)
        {
            //todo die
        }
        upperBound = lowerBound = new double(oldT * mul);
    }
}