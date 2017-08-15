#include "SymbolicVariables.h"

using namespace std;

SymbolicString::SymbolicString(string name, Reporter &reporter):
    SymbolicVariable(name, "", "", reporter, STRING),
    boundedLower(true), boundedUpper(true) {}

SymbolicString::SymbolicString(SymbolicString& other):
    SymbolicVariable(other.varN, other.lowerBound, other.upperBound, other.reporter, STRING),
    boundedLower(other.isBoundedBelow()), boundedUpper(other.isBoundedAbove()) {}

SymbolicString::SymbolicString(shared_ptr<SymbolicString> other):
    SymbolicString(other.get()) {}

void SymbolicString::userInput()
{
    boundedLower = boundedUpper = false;
}

bool SymbolicString::isBoundedBelow() const
{
    return boundedLower;
}

bool SymbolicString::isBoundedAbove() const
{
    return boundedUpper;
}