#include <limits>
#include <algorithm>
#include <math.h>

#include "SymbolicVariables.h"

using namespace std;

//SymbolicVariable
template <typename T>
SymbolicVariable<T>::SymbolicVariable(string name, const T lower, const T upper,
                                      Reporter& r, VariableType t, bool init): reporter(r)
{
    lowerBound = lower;
    upperBound = upper;
    type = t;
    isConst = lowerBound == upperBound;
    feasable = lowerBound <= upperBound;
    isInitialised = init;
}

template <typename T>
const T SymbolicVariable<T>::getUpperBound() const
{
    return upperBound;
}


template <typename T>
const T SymbolicVariable<T>::getLowerBound() const
{
    return lowerBound;
}

template <typename T>
const T SymbolicVariable<T>::getConstValue() const
{
    if (!isConst) throw "Not constant";
    return lowerBound; //could be upper
}

template <typename T>
void SymbolicVariable<T>::setLowerBound(const T lower)
{
    lowerBound = lower;
    isConst = lowerBound == upperBound;
}

template <typename T>
void SymbolicVariable<T>::setUpperBound(const T upper)
{
    upperBound = upper;
    isConst = lowerBound == upperBound;
}

template <typename T>
void SymbolicVariable<T>::setConstValue(const T c)
{
    upperBound = lowerBound = c;
    isConst = true;
    isInitialised = true;
}

template <typename T>
bool SymbolicVariable<T>::isDetermined() const
{
    return isConst;
}

template <typename T>
const string SymbolicVariable<T>::getName() const
{
    return varN;
}

template <typename T>
void SymbolicVariable<T>::reportError(Reporter::AlertType type, string err)
{
    reporter.error(type, err);
    feasable = false;
}

template <typename T>
const VariableType SymbolicVariable<T>::getType() const
{
    return type;
}

//SymbolicVariablePointer
SymbolicVariablePointer::SymbolicVariablePointer(shared_ptr<SymbolicDouble> dptr):
    type(DOUBLE), doublePtr(dptr) {}

SymbolicVariablePointer::SymbolicVariablePointer(shared_ptr<SymbolicString> sptr):
    type(STRING), stringPtr(sptr) {}

SymbolicVariablePointer::SymbolicVariablePointer(SymbolicVariablePointer& other):
    type(other.type)
{
    if (type == DOUBLE) doublePtr = make_shared<SymbolicDouble>(other.doublePtr);
    else stringPtr = make_shared<SymbolicString>(other.doublePtr);
}

SymbolicVariablePointer::SymbolicVariablePointer(shared_ptr<SymbolicVariablePointer> other):
    SymbolicVariablePointer(*other.get()) {}

SymbolicVariablePointer::SymbolicVariablePointer(): type(VOID) {}

SymbolicVariablePointer::~SymbolicVariablePointer()
{
    if (type == DOUBLE) doublePtr.reset();
    else stringPtr.reset();
}

VariableType SymbolicVariablePointer::getType()
{
    return type;
}

shared_ptr<SymbolicDouble> SymbolicVariablePointer::getDoublePointer()
{
    if (type != DOUBLE) throw runtime_error("Tried to get double from something that is not a double");
    return doublePtr;
}

shared_ptr<SymbolicString> SymbolicVariablePointer::getStringPointer()
{
    if (type != STRING) throw runtime_error("Tried to get string from something that is not a string");
    return stringPtr;
}

bool SymbolicVariablePointer::isNull()
{
    if (type == STRING) return stringPtr == nullptr;
    else if (type == DOUBLE) return doublePtr == nullptr;
    else return true;
}