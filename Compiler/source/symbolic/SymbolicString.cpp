#include <limits>

#include "SymbolicVariables.h"

using namespace std;

SymbolicString::SymbolicString(string name, Reporter& reporter):
    SymbolicVariableTemplate(move(name), "", "", reporter, STRING, false, false),
    boundedLower(true), boundedUpper(true) {}

SymbolicString::SymbolicString(SymbolicString* other):
        SymbolicVariableTemplate(other->varN, other->lowerBound, other->upperBound,
                                 other->reporter, STRING, other->isDefined() ,other->isIncrementable()),
        boundedLower(other->isBoundedBelow()), boundedUpper(other->isBoundedAbove()){}


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
void SymbolicString::unionTConstValue(const string& cv, bool closed)
{
    if (closed)
    {
        if (boundedLower && cv < lowerBound) lowerBound = cv;
        else if (boundedUpper && cv > upperBound) upperBound = cv;
    }
    else
    {
        if (boundedLower && cv <= lowerBound) lowerBound = decrementString(cv);
        else if (boundedUpper && cv >= upperBound) upperBound = incrementString(cv);
    }
}
void SymbolicString::setConstValue(const string &s)
{
    setTConstValue(s);
}

bool SymbolicString::setTLowerBound(const string& lb, bool closed)
{
    if (!closed) upperBound = decrementString(lb);
    else lowerBound = lb;
    if (upperBound > lowerBound) feasable = false;
    boundedLower = true;
    return isFeasable();
}
bool SymbolicString::setLowerBound(const string& lb, bool closed)
{
    return setTLowerBound(lb, closed);
}

bool SymbolicString::setTUpperBound(const string& ub, bool closed)
{
    if (!closed) upperBound = incrementString(ub);
    else upperBound = ub;
    if (upperBound > lowerBound) feasable = false;
    boundedUpper = true;
    return isFeasable();
}
bool SymbolicString::setUpperBound(const string& ub, bool closed)
{
    return setTUpperBound(ub, closed);
}

bool SymbolicString::clipTLowerBound(const string& lb, bool closed)
{
    if (!isBoundedBelow() || getLowerBound() > lb) return setLowerBound(lb, closed);
    else return isFeasable();
}
bool SymbolicString::clipLowerBound(const string& lb, bool closed)
{
    return clipTLowerBound(lb, closed);
}

bool SymbolicString::clipTUpperBound(const string& ub, bool closed)
{
    if (!isBoundedAbove() || getUpperBound() < ub) return setLowerBound(ub, closed);
    else return isFeasable();
}
bool SymbolicString::clipUpperBound(const string& ub, bool closed)
{
    return clipTUpperBound(ub, closed);
}

bool SymbolicString::unionTLowerBound(const string& lb, bool closed)
{
    if (!isBoundedBelow() && lb < lowerBound)
    {
        setLowerBound(lb, closed);
        return true;
    }
    else return false;
}
bool SymbolicString::unionLowerBound(const string& lb, bool closed)
{
    return unionTLowerBound(lb);
}

bool SymbolicString::unionTUpperBound(const string& ub, bool closed)
{
    if (!isBoundedAbove() && ub > upperBound)
    {
        setUpperBound(ub, closed);
        return true;
    }
    else return false;
}
bool SymbolicString::unionUpperBound(const string& ub, bool closed)
{
    return unionTUpperBound(ub);
}

bool SymbolicString::unionVar(SymbolicVariable* other)
{
    if (other->getType() != STRING) throw "wrong";
    SymbolicString* sd = static_cast<SymbolicString*>(other);
    bool ret = unionTLowerBound(sd->getTUpperBound());
    if (unionTLowerBound(sd->getTLowerBound())) ret = true;
    return ret;
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

SymbolicVariable::MeetEnum SymbolicString::canMeet(Relations::Relop rel, const string& rhs) const
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

string SymbolicString::incrementString(string s)
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

string SymbolicString::decrementString(string s)
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