#include <limits>
#include <algorithm>
#include <math.h>

#include "SymbolicVariables.h"

using namespace std;

//SymbolicVariable

SymbolicVariable::SymbolicVariable(std::string name, VariableType t, Reporter &r, bool initialised):
        varN(name), type(t), reporter(r), isInitialised(initialised) {}

const string SymbolicVariable::getName() const
{
    return varN;
}

void SymbolicVariable::reportError(Reporter::AlertType type, string err)
{
    reporter.error(type, err);
    feasable = false;
}

bool SymbolicVariable::isDetermined()
{
    return isConst;
}

const VariableType SymbolicVariable::getType() const
{
    return type;
}

//SymbolicVariableTemplate
template <typename T>
SymbolicVariableTemplate<T>::SymbolicVariableTemplate(string name, const T lower, const T upper,
                                      Reporter& r, VariableType t, bool init): SymbolicVariable(name, t, r, init)
{
    lowerBound = lower;
    upperBound = upper;
}

template <typename T>
const T SymbolicVariableTemplate<T>::getUpperBound() const
{
    return upperBound;
}

template <typename T>
bool SymbolicVariableTemplate<T>::isFeasable()
{
    if (upperBound > lowerBound) feasable = false;
    return feasable;
}

template <typename T>
const T SymbolicVariableTemplate<T>::getLowerBound() const
{
    return lowerBound;
}

template <typename T>
const T SymbolicVariableTemplate<T>::getConstValue()
{
    if (!isDetermined()) throw "Not constant";
    return lowerBound; //could be upper
}

template <typename T>
bool SymbolicVariableTemplate<T>::isDetermined()
{
    if (lowerBound == upperBound) isConst = true;
    return isConst;
}

template <typename T>
void SymbolicVariableTemplate<T>::setLowerBound(const T lower)
{
    lowerBound = lower;
    isConst = lowerBound == upperBound;
}

template <typename T>
void SymbolicVariableTemplate<T>::setUpperBound(const T upper)
{
    upperBound = upper;
    isConst = lowerBound == upperBound;
}

template <typename T>
void SymbolicVariableTemplate<T>::setConstValue(const T c)
{
    upperBound = lowerBound = c;
    isConst = true;
    isInitialised = true;
}

template class SymbolicVariableTemplate<double>;
template class SymbolicVariableTemplate<std::string>;

/*SymbolicVariablePointer
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
}*/