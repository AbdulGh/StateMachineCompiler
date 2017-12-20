#include <limits>

#include "SymbolicVariables.h"

using namespace std;

SymbolicString::SymbolicString(string name, Reporter &reporter):
    SymbolicVariableTemplate(move(name), "", "", reporter, STRING),
    boundedLower(true), boundedUpper(true) {}

SymbolicString::SymbolicString(SymbolicString* other):
        SymbolicVariableTemplate(other->varN, other->lowerBound, other->upperBound, other->reporter, STRING),
        boundedLower(other->isBoundedBelow()), boundedUpper(other->isBoundedAbove()) {}


SymbolicString::SymbolicString(SymbolicVariable* other):
        SymbolicString(static_cast<SymbolicString*>(other)) {}

unique_ptr<SymbolicVariable> SymbolicString::clone()
{
    return make_unique<SymbolicString>(*this);
}

SymbolicVariable* SymbolicString::cloneRaw()
{
    return new SymbolicString(*this);
}

void SymbolicString::setTConstValue(const string& cv)
{
    boundedUpper = boundedLower = true;
    lowerBound = upperBound = cv;
}
void SymbolicString::setConstValue(const std::string &s)
{
    setTConstValue(s);
}

bool SymbolicString::setTLowerBound(const std::string& lb, bool closed)
{
    if (!closed) upperBound = decrementString(lb);
    else lowerBound = lb;
    if (upperBound > lowerBound) feasable = false;
    boundedLower = true;
    return isFeasable();
}
bool SymbolicString::setLowerBound(const std::string& lb, bool closed)
{
    return setTLowerBound(lb, closed);
}

bool SymbolicString::setTUpperBound(const std::string& ub, bool closed)
{
    if (!closed) upperBound = incrementString(ub);
    else upperBound = ub;
    if (upperBound > lowerBound) feasable = false;
    boundedUpper = true;
    return isFeasable();
}
bool SymbolicString::setUpperBound(const std::string& ub, bool closed)
{
    return setTUpperBound(ub, closed);
}

bool SymbolicString::clipTLowerBound(const std::string& lb, bool closed)
{
    if (!isBoundedBelow() || getLowerBound() > lb) return setLowerBound(lb, closed);
    else return isFeasable();
}
bool SymbolicString::clipLowerBound(const std::string& lb, bool closed)
{
    return clipTLowerBound(lb, closed);
}

bool SymbolicString::clipTUpperBound(const std::string& ub, bool closed)
{
    if (!isBoundedAbove() || getUpperBound() < ub) return setLowerBound(ub, closed);
    else return isFeasable();
}
bool SymbolicString::clipUpperBound(const std::string& ub, bool closed)
{
    return clipTUpperBound(ub, closed);
}

bool SymbolicString::unionTLowerBound(const std::string& lb, bool closed)
{
    if (!isBoundedBelow() && lb < lowerBound) return setLowerBound(lb, closed);
    else return isFeasable();
}
bool SymbolicString::unionLowerBound(const std::string& lb, bool closed)
{
    return unionTLowerBound(lb);
}

bool SymbolicString::unionTUpperBound(const std::string& ub, bool closed)
{
    if (!isBoundedAbove() && ub > lowerBound) return setUpperBound(ub, closed);
    else return isFeasable();
}
bool SymbolicString::unionUpperBound(const std::string& ub, bool closed)
{
    return unionTUpperBound(ub);
}

void SymbolicString::userInput()
{
    SymbolicVariable::userInput();
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
    if (isDetermined()) return (Relations::evaluateRelop<string>(getTLowerBound(), rel, rhs)) ? MUST : CANT;

    else
    {
        bool leLower = isBoundedBelow() && rhs <= getTLowerBound();
        bool geLower = !isBoundedBelow() || rhs >= getTLowerBound();
        bool leUpper = !isBoundedAbove() || rhs <= getTUpperBound();
        bool geUpper = isBoundedAbove() && rhs >= getTUpperBound();
        bool neqLower = rhs != getTLowerBound();
        bool neqUpper = rhs != getTUpperBound();

        switch(rel)
        {
            case Relations::EQ:
                return (leLower && neqLower) || (geUpper && neqUpper) ? CANT : MAY;
            case Relations::NEQ:
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