#include "SymbolicVariables.h"

using namespace std;

SymbolicString::SymbolicString(string name, Reporter &reporter):
    SymbolicVariableTemplate(name, "", "", reporter, STRING),
    boundedLower(true), boundedUpper(true) {}

SymbolicString::SymbolicString(SymbolicString& other):
    SymbolicVariableTemplate(other.varN, other.lowerBound, other.upperBound, other.reporter, STRING),
    boundedLower(other.isBoundedBelow()), boundedUpper(other.isBoundedAbove()) {}

SymbolicString::SymbolicString(shared_ptr<SymbolicString> other):
    SymbolicString(*other.get()) {}

SymbolicString::SymbolicString(shared_ptr<SymbolicVariable> other):
        SymbolicString(static_pointer_cast<SymbolicString>(other)) {}

void SymbolicString::setUpperBound(const std::string& ub)
{
    upperBound = ub;
    if (upperBound > lowerBound) feasable = false;
    if (upperBound == lowerBound) isConst = true;
    boundedUpper = true;
}

void SymbolicString::setLowerBound(const std::string& lb)
{
    lowerBound = lb;
    if (upperBound > lowerBound) feasable = false;
    if (upperBound == lowerBound) isConst = true;
    boundedLower = true;
}

void SymbolicString::setConstValue(const std::string& c)
{
    lowerBound = upperBound = c;
    isConst = true;
}

void SymbolicString::userInput()
{
    boundedLower = boundedUpper = false;
    lowerBound = upperBound = "";
}

bool SymbolicString::isBoundedBelow() const
{
    return boundedLower;
}

bool SymbolicString::isBoundedAbove() const
{
    return boundedUpper;
}

bool SymbolicString::canMeet(Relations::Relop rel, std::string rhs) const override
{
    bool belowLower = !isBoundedBelow() //todo continue here I

    switch(rel)
    {
        case Relations::EQ:
            return rhs >= getLowerBound()) && (!is rhs <= getLowerBound();
        case Relations::NE:
            return !isConst || getUpperBound() != rhs;
        case Relations::LE:
            return getLowerBound() <= rhs;
        case Relations::LT:
            return getLowerBound() < rhs;
        case Relations::GE:
            return getUpperBound() >= rhs;
        case Relations::GT:
            return getUpperBound() > rhs;
    }
}