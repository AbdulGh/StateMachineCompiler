#include <limits>

#include "SymbolicVariables.h"

using namespace std;

SymbolicString::SymbolicString(string name, Reporter &reporter):
    SymbolicVariableTemplate(move(name), "", "", reporter, STRING),
    boundedLower(true), boundedUpper(true) {}

SymbolicString::SymbolicString(SymbolicString& other):
    SymbolicVariableTemplate(other.varN, other.lowerBound, other.upperBound, other.reporter, STRING),
    boundedLower(other.isBoundedBelow()), boundedUpper(other.isBoundedAbove()) {}

SymbolicString::SymbolicString(shared_ptr<SymbolicString> other):
    SymbolicString(*other.get()) {}

SymbolicString::SymbolicString(shared_ptr<SymbolicVariable> other):
        SymbolicString(static_pointer_cast<SymbolicString>(other)) {}

shared_ptr<SymbolicVariable> SymbolicString::clone()
{
    return make_shared<SymbolicString>(this);
}

bool SymbolicString::setUpperBound(const std::string& ub, bool closed)
{
    if (!closed)
    {
        string copy(ub);
        upperBound = incrementString(copy);
    }
    else upperBound = ub;
    if (upperBound > lowerBound) feasable = false;
    if (upperBound == lowerBound) isConst = true;
    boundedUpper = true;
    return isFeasable();
}

void SymbolicString::setConstValue(const string& cv)
{
    boundedUpper = boundedLower = true;
    lowerBound = upperBound = cv;
    isConst = true;
}
void SymbolicString::setStringConstValue(const std::string &s)
{
    setConstValue(s);
}

bool SymbolicString::setLowerBound(const std::string& lb, bool closed)
{
    if (!closed) upperBound = decrementString(lb);
    else lowerBound = lb;
    if (upperBound > lowerBound) feasable = false;
    if (upperBound == lowerBound) isConst = true;
    boundedLower = true;
    return isFeasable();
}

bool SymbolicString::clipLowerBound(const std::string& lb, bool closed)
{
    if (!isBoundedBelow() || getLowerBound() > lb) return setLowerBound(lb, closed);
    else return isFeasable();
}
bool SymbolicString::clipStringLowerBound(const std::string& lb, bool closed)
{
    return clipLowerBound(lb, closed);
}

bool SymbolicString::clipUpperBound(const std::string& ub, bool closed)
{
    if (!isBoundedAbove() || getUpperBound() < ub) return setLowerBound(ub, closed);
    else return isFeasable();
}
bool SymbolicString::clipStringUpperBound(const std::string& ub, bool closed)
{
    return clipUpperBound(ub, closed);
}

bool SymbolicString::unionLowerBound(const std::string& lb, bool closed)
{
    if (!isBoundedBelow() && lb < lowerBound) return setLowerBound(lb, closed);
    else return isFeasable();
}
bool SymbolicString::unionStringLowerBound(const std::string& lb, bool closed)
{
    return unionLowerBound(lb);
}

bool SymbolicString::unionUpperBound(const std::string& ub, bool closed)
{
    if (!isBoundedAbove() && ub > lowerBound) return setUpperBound(ub, closed);
    else return isFeasable();
}
bool SymbolicString::unionStringUpperBound(const std::string& ub, bool closed)
{
    return unionUpperBound(ub);
}

void SymbolicString::userInput()
{
    boundedLower = boundedUpper = false;
    lowerBound = upperBound = "";
    define();
}

bool SymbolicString::isBoundedBelow() const
{
    return boundedLower;
}

bool SymbolicString::isBoundedAbove() const
{
    return boundedUpper;
}

SymbolicVariable::MeetEnum SymbolicString::canMeet(Relations::Relop rel, const std::string& rhs) const
{
    if (isConst) return (Relations::evaluateRelop<string>(getLowerBound(), rel, rhs)) ? MUST : CANT;

    else
    {
        bool leLower = isBoundedBelow() && rhs <= getLowerBound();
        bool geLower = !isBoundedBelow() || rhs >= getLowerBound();
        bool leUpper = !isBoundedAbove() || rhs <= getUpperBound();
        bool geUpper = isBoundedAbove() && rhs >= getUpperBound();
        bool neqLower = rhs != getLowerBound();
        bool neqUpper = rhs != getUpperBound();

        switch(rel)
        {
            case Relations::EQ:
                return (leLower && neqLower) || (geUpper && neqUpper) ? CANT : MAY;
            case Relations::NE:
                return (leLower && neqLower) || (geUpper && neqUpper) ? MUST : MAY;
            case Relations::LE:
                if (leLower) return MUST;
                else if (leUpper) return MAY;
                return CANT;
            case Relations::LT:
                if (leLower && neqLower) return MUST;
                else if (leUpper) return MAY;
                return CANT;
            case Relations::GE:
                if (geUpper) return MUST;
                else if (geLower) return MAY;
                return CANT;
            case Relations::GT:
                if (geUpper && neqUpper) return MUST;
                else if (geLower) return MAY;
                return CANT;
            default:
                throw runtime_error("bad relop");
        }
    }
}

std::string SymbolicString::incrementString(std::string s)
{
    for (unsigned long index = s.length() - 1; index != 0; --index)
    {
        if (s[index] == numeric_limits<char>::max()) s[index] = numeric_limits<char>::lowest();
        else
        {
            ++s[index];
            return s;
        }
    }
    return numeric_limits<char>::max() + s;
}

std::string SymbolicString::decrementString(std::string s)
{
    for (unsigned long index = s.length() - 1; index != 0; --index)
    {
        if (s[index] == numeric_limits<char>::lowest()) s[index] = numeric_limits<char>::max();
        else
        {
            --s[index];
            return s;
        }
    }
    return numeric_limits<char>::lowest() + s;
}