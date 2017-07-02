#include "SymbolicVariable.h"

template <typename T>
void SymbolicVariable<T>::clipLower(T t)
{
    if (lowerBound == nullptr || lowerBound < t) lowerBound = t;
}

template <typename T>
void SymbolicVariable<T>::clipUpper(T t)
{
    if (upperBound == nullptr || upperBound > t) upperBound = t;
}

template <typename T>
bool SymbolicVariable<T>::isFeasable()
{
    return lowerBound <= upperBound;
}

template <typename T>
T SymbolicVariable<T>::getLowerBound() const
{
    return lowerBound;
}

template <typename T>
T SymbolicVariable<T>::getUpperBound() const
{
    return upperBound;
}
