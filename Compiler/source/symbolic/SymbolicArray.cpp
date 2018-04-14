#include "SymbolicExecution.h"
#include "SymbolicArray.h"

bool SymbolicArray::set(SymbolicDouble* index, SymbolicDouble* sdr, int linenum)
{
    if (index->isDetermined())
    {
        double intpart;
        if (modf(index->getConstValue(), &intpart) != 0.0)
        {
            parent.reporter.error(Reporter::ARRAY_BOUNDS, "Asked to get non integral index '"
                                                          + std::to_string(index->getConstValue())
                                                          + "' in array", linenum);
            return false;
        }
        return set(intpart, sdr, linenum);
    }

    double lb, ub;
    if (!checkBounds(index->getLowerBound(), index->getUpperBound(), lb, ub, linenum)) return false;

    auto it = indicies.begin();
    auto varit = indexVars.begin();

    if (lb != 0)
    {
        while (*it <= lb)
        {
            ++it;
            ++varit;
            if (it == indicies.end() || varit == indexVars.end()) throw std::runtime_error("bad");
        }
        it = indicies.insert(it, lb);
        std::unique_ptr<SymbolicDouble> newD = (*varit)->clone();
        varit = indexVars.insert(varit, newD.get());
        myVars.push_back(move(newD));
        ++it; ++varit;
    }

    while (it != indicies.end() && varit != indexVars.end())
    {
        if (*it == ub + 1)
        {
            **varit = *sdr;
            return true;
        }
        else if (*it > ub)
        {
            indicies.insert(it, ub + 1);
            std::unique_ptr<SymbolicDouble> newSD = (*varit)->clone();
            *newSD = *sdr;
            indexVars.insert(varit, newSD.get());
            myVars.push_back(move(newSD));
            return true;
        }
        else
        {
            **varit = *sdr;
            ++it;
            ++varit;
        }
    }
    throw std::runtime_error("shouldn't get here");
}

bool SymbolicArray::set(unsigned int n, std::unique_ptr<SymbolicDouble> sd, int linenum)
{
    if (n >= size)
    {
        if (!warnedUpper)
        {
            warnedUpper = true;
            parent.reporter.error(Reporter::ARRAY_BOUNDS,
                                  "Asked to set index " + std::to_string(n) + " in array of size " + std::to_string(size),
                                  linenum);
        }
        return false;
    }

    auto it = indicies.begin();
    auto varit = indexVars.begin();
    while (it != indicies.end() && varit != indexVars.end())
    {
        if (*it > n)
        {
            it = indicies.insert(it, n);
            ++it;
            if (*it > n + 1)
            {
                indicies.insert(it, n+1);
                varit = indexVars.insert(varit, *varit);
                ++varit;
            }
            indexVars.insert(varit, sd.get());
            myVars.push_back(move(sd));
            return true;
        }
        else
        {
            ++it;
            ++varit;
        }
    }
    throw std::runtime_error("shouldnt reach here");
}

std::unique_ptr<SymbolicDouble> SymbolicArray::get(unsigned int n, int linenum)
{
    if (n >= size)
    {
        if (!warnedUpper)
        {
            warnedUpper = true;
            parent.reporter.error(Reporter::ARRAY_BOUNDS,
                                  "Asked to get index " + std::to_string(n) + " in array of size " + std::to_string(size),
                                  linenum);
        }
        return nullptr;
    }
    if (n < size)
    {
        if (!warnedLower)
        {
            warnedLower = true;
            parent.reporter.error(Reporter::ARRAY_BOUNDS,
                                  "Asked to get index " + std::to_string(n) + " in array of size " + std::to_string(size),
                                  linenum);
        }
        return nullptr;
    }

    auto it = indicies.begin();
    auto varit = indexVars.begin();
    while (it != indicies.end() && varit != indexVars.end())
    {
        if (*it > n) return (*varit)->clone();
        else
        {
            ++it;
            ++varit;
        }
    }
    throw std::runtime_error("shouldn't happen");
}

bool SymbolicArray::checkIndex(long index, int linenum)
{
    if (index < 0)
    {
        if (!warnedLower)
        {
            parent.reporter.error(Reporter::ARRAY_BOUNDS, "Asked for negative index from array", linenum);
            warnedLower = true;
        }
        return false;
    }
    else if (index >= size)
    {
        if (!warnedUpper)
        {
            warnedUpper = true;
            parent.reporter.error(Reporter::ARRAY_BOUNDS,
                                  "Asked to get index " + std::to_string(index) + " in array of size " +
                                  std::to_string(size),
                                  linenum);
        }
        return false;
    }
    return true;
}

bool SymbolicArray::checkBounds(double lbd, double ubd, double& lb, double& ub, int linenum)
{
    lb = ceil(lbd);
    ub = floor(ubd);

    if (ub < 0)
    {
        std::ostringstream precision;
        precision << std::setprecision(10);
        precision << ub;
        parent.reporter.error(Reporter::ARRAY_BOUNDS,
                              "Asked to get index <= " + precision.str() + " in array", linenum);
        return false;
    }
    if (lb >= size)
    {
        std::ostringstream precision;
        precision << std::setprecision(10);
        precision << lb;
        if (!warnedUpper)
        {
            warnedUpper = true;
            parent.reporter.error(Reporter::ARRAY_BOUNDS,
                                  "Asked to get index >= " + precision.str() + " in array of size " + std::to_string(size),
                                  linenum);
        }
        return false;
    }

    if (lb < 0)
    {
        parent.reporter.warn(Reporter::ARRAY_BOUNDS, "Asked to access possibly negative index", linenum);
        lb = 0;
    }
    if (ub >= size)
    {
        std::ostringstream precision;
        precision << std::setprecision(10);
        precision << ub;

        if (!warnedUpper)
        {
            warnedUpper = true;
            parent.reporter.warn(Reporter::ARRAY_BOUNDS,
                                 "Could access index up to " + precision.str() + " in array of size " +
                                 std::to_string(size),
                                 linenum);
        }
        ub = size - 1;
    }

    return true;
}

bool SymbolicArray::checkBounds(double lbd, double ubd, int linenum)
{
    lbd = ceil(lbd);
    ubd = floor(ubd);

    if (ubd < 0)
    {
        if (!warnedLower)
        {
            warnedLower = true;
            parent.reporter.error(Reporter::ARRAY_BOUNDS,
                                  "Asked to get index <= " + std::to_string(ubd) + " in array", linenum);
        }
        return false;
    }
    if (lbd >= size)
    {
        if (!warnedUpper)
        {
            warnedUpper = true;
            parent.reporter.error(Reporter::ARRAY_BOUNDS,
                                  "Asked to get index >= " + std::to_string(lbd) + " in array of size " +
                                  std::to_string(size),
                                  linenum);
        }
        return false;
    }

    if (lbd < 0 && !warnedLower)
    {
        warnedLower = true;
        parent.reporter.warn(Reporter::ARRAY_BOUNDS, "Asked to access possibly negative index", linenum);
    }

    if (ubd >= size && !warnedUpper)
    {
        warnedUpper = true;
        std::ostringstream precision;
        precision << std::setprecision(10);
        precision << ubd;
        parent.reporter.warn(Reporter::ARRAY_BOUNDS,
                             "Could access index up to " + precision.str() + " in array of size " + std::to_string(size),
                             linenum);
    }

    return true;
}