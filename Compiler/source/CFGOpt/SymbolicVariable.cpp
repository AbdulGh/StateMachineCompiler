#include "SymbolicVariable.h"

template <typename T>
SymbolicVariable<T>::SymbolicVariable():
    upperBound(nullptr),
    lowerBound(nullptr),
    minStep(nullptr),
    monotinicity(FRESH){}

template <typename T>
void SymbolicVariable<T>::reset()
{
    if (upperBound != nullptr) delete upperBound;
    upperBound = nullptr;
    if (lowerBound != nullptr) delete lowerBound;
    lowerBound = nullptr;
    if (minStep != nullptr) delete minStep;
    minStep = nullptr;
    monotinicity = FRESH;
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
const SymbolicVariable<T>::MonotoneEnum SymbolicVariable<T>::getMonotinicity() const
{
    return monotinicity;
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
const T SymbolicVariable::getConstValue() const
{
    if (monotinicity != CONST) throw "Not constant";
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
    monotinicity = CONST;
}

template <typename T>
bool SymbolicVariable<T>::isDetermined() const
{
    return (monotinicity == CONST || monotinicity == INCREASING
            || monotinicity == DECREASING || monotinicity == UNKNOWN);
}

template <typename T>
void SymbolicVariable<T>::addToConst(const T& diff)
{
    if (!isDetermined()) throw "Cannot add to undetermined variable";
    if (diff == 0) return;

    if (monotinicity == CONST)
    {
        monotinicity = (diff > 0) ? INCREASING : DECREASING;
        minStep = new T(diff);
    }
    else if (monotinicity == INCREASING)
    {
        if (diff < 0) monotinicity = UNKNOWN;
        else if (diff < *minStep)
        {
            delete minStep;
            minStep = new T(diff);
        }
    }
    else if (monotinicity == DECREASING)
    {
        if (diff > 0) monotinicity = UNKNOWN;
        else if (diff > *minStep)
        {
            delete minStep;
            minStep = new T(diff);
        }
    }

    T oldT = *lowerBound;
    delete lowerBound;
    upperBound = lowerBound = new T(oldT + diff);
}