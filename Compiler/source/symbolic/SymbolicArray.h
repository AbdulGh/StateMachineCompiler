//
// Created by abdul on 03/02/18.
//

#ifndef PROJECT_SYMBOLICARRAY_H
#define PROJECT_SYMBOLICARRAY_H

#include <list>
#include <map>
#include <iomanip>

#include "SymbolicDouble.h"

class SymbolicArray
{
private:
    std::vector<std::unique_ptr<SymbolicDouble>> myVars;
    //<u_0, u_1... > where vars[i] is in indicies [u_{i-1}, u_i)
    std::list<int> indicies;
    std::list<SymbolicDouble*> indexVars;
    const unsigned int size;
    Reporter& reporter;
    std::string name;

public:
    SymbolicArray(std::string id, unsigned int n, Reporter& r): size(n), reporter(r), name(std::move(id))
    {
        myVars.emplace_back(std::make_unique<SymbolicDouble>("uninit", reporter));
        indicies.push_back(size);
        indexVars.push_back(myVars.begin()->get());
    }

    SymbolicArray(const SymbolicArray& other): reporter(other.reporter), size(other.size)
    {
        indicies = other.indicies;

        std::map<SymbolicDouble*, SymbolicDouble*> cloneMap;
        myVars.reserve(other.myVars.size());

        for (const auto& otherVar : other.myVars)
        {
            std::unique_ptr<SymbolicDouble> clone = otherVar->clone();
            cloneMap[otherVar.get()] = clone.get();
            myVars.push_back(move(clone));
        }

        for (SymbolicDouble* ind : other.indexVars) indexVars.push_back(cloneMap[ind]);
    }

    ~SymbolicArray()
    {
        for (auto& var : myVars) var.reset();
    }

    std::unique_ptr<SymbolicArray> clone()
    {
        return std::make_unique<SymbolicArray>(*this);
    }
    
    bool checkIndex(long index)
    {
        if (index < 0) reporter.error(Reporter::ARRAY_BOUNDS, "Asked for negative index from array");
        else if (index >= size)
        {
            reporter.error(Reporter::ARRAY_BOUNDS,
                           "Asked to get index " + std::to_string(index) + " in array of size " + std::to_string(size));
        }
    }
    
    bool checkBounds(double lbd, double ubd, double& lb, double& ub)
    {
        lb = ceil(lbd);
        ub = floor(ubd);

        if (ub < 0)
        {
            std::ostringstream precision;
            precision << std::setprecision(10);
            precision << ub;
            reporter.error(Reporter::ARRAY_BOUNDS,
                           "Asked to get index <= " + precision.str() + " in array");
            return false;
        }
        if (lb >= size)
        {
            std::ostringstream precision;
            precision << std::setprecision(10);
            precision << lb;
            reporter.error(Reporter::ARRAY_BOUNDS,
                           "Asked to get index >= " + precision.str() + " in array of size " + std::to_string(size));
            return false;
        }

        if (lb < 0)
        {
            reporter.warn(Reporter::ARRAY_BOUNDS, "Asked to access possibly negative index");
            lb = 0;
        }
        if (ub >= size)
        {
            std::ostringstream precision;
            precision << std::setprecision(10);
            precision << ub;

            reporter.warn(Reporter::ARRAY_BOUNDS,
                          "Could access index up to " + precision.str() + " in array of size " + std::to_string(size));
            ub = size - 1;
        }
        
        return true;
    }

    bool checkBounds(double lbd, double ubd)
    {
        lbd = ceil(lbd);
        ubd = floor(ubd);

        if (ubd < 0)
        {
            reporter.error(Reporter::ARRAY_BOUNDS,
                           "Asked to get index <= " + std::to_string(ubd) + " in array");
            return false;
        }
        if (lbd >= size)
        {
            reporter.error(Reporter::ARRAY_BOUNDS,
                           "Asked to get index >= " + std::to_string(lbd) + " in array of size " + std::to_string(size));
            return false;
        }

        if (lbd < 0) reporter.warn(Reporter::ARRAY_BOUNDS, "Asked to access possibly negative index");

        if (ubd >= size)
        {
            std::ostringstream precision;
            precision << std::setprecision(10);
            precision << ubd;
            reporter.warn(Reporter::ARRAY_BOUNDS,
                          "Could access index up to " + precision.str() + " in array of size " + std::to_string(size));
        }

        return true;
    }

    std::unique_ptr<SymbolicDouble> operator[](unsigned int n)
    {
        if (n >= size)
        {
            reporter.error(Reporter::ARRAY_BOUNDS,
                           "Asked to get index " + std::to_string(n) + " in array of size " + std::to_string(size));
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

    std::unique_ptr<SymbolicDouble> operator[](SymbolicDouble* index)
    {
        double lb, ub;

        if (!checkBounds(index->getLowerBound(), index->getUpperBound(), lb, ub)) return nullptr;

        auto it = indicies.begin();
        auto varit = indexVars.begin();

        while (*it <= lb)
        {
            ++it;
            ++varit;
            if (it == indicies.end() || varit == indexVars.end()) throw std::runtime_error("bad");
        }

        std::unique_ptr<SymbolicDouble> cp = (*varit)->clone();

        while (it != indicies.end() || varit != indexVars.end())
        {
            if (ub >= *it)
            {
                ++varit;
                ++it;
                cp->unionVar(*varit);
            }
            else return std::move(cp);
        }

        throw std::runtime_error("shouldn't get here");
    }

    bool set(unsigned int n, double v)
    {
        std::unique_ptr<SymbolicDouble> sd = std::make_unique<SymbolicDouble>("constDouble", reporter);
        sd->setConstValue(v);
        set(n, std::move(sd));
    }

    bool set(unsigned int n, std::unique_ptr<SymbolicDouble> sd)
    {
        if (n >= size)
        {
            reporter.error(Reporter::ARRAY_BOUNDS,
                           "Asked to set index " + std::to_string(n) + " in array of size " + std::to_string(size));
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

    bool set(unsigned int n, SymbolicDouble* sdr)
    {
        std::unique_ptr<SymbolicDouble> sd = sdr->clone();
        set(n, std::move(sd));
    }

    bool set(SymbolicDouble* index, SymbolicDouble* sdr)
    {
        if (index->isDetermined())
        {
            double intpart;
            if (modf(index->getConstValue(), &intpart) != 0.0)
            {
                reporter.error(Reporter::ARRAY_BOUNDS, "Asked to get non integral index '"
                               + std::to_string(index->getConstValue())
                               + "' in array");
                return false;
            }
            return set(intpart, sdr);
        }
        double lb = ceil(index->getLowerBound());
        double ub = floor(index->getUpperBound());
        if (ub < 0)
        {
            std::ostringstream precision;
            precision << std::setprecision(10);
            precision << ub;
            reporter.error(Reporter::ARRAY_BOUNDS,
                           "Asked to set index <= " + precision.str() + " in array");
            return false;
        }
        if (lb >= size)
        {
            std::ostringstream precision;
            precision << std::setprecision(10);
            precision << lb;
            reporter.error(Reporter::ARRAY_BOUNDS,
                           "Asked to set index >= " + precision.str() + " in array of size " + std::to_string(size));
            return false;
        }

        if (lb < 0)
        {
            reporter.warn(Reporter::ARRAY_BOUNDS, "Asked to access possibly negative index");
            lb = 0;
        }
        if (ub >= size)
        {
            std::ostringstream precision;
            precision << std::setprecision(10);
            precision << ub;
            reporter.warn(Reporter::ARRAY_BOUNDS,
                          "Could access index up to " + precision.str() + " in array of size " + std::to_string(size));
            ub = size - 1;
        }

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
                (*varit)->unionVar(sdr);
                return true;
            }
            else if (*it > ub)
            {
                indicies.insert(it, ub + 1);
                std::unique_ptr<SymbolicDouble> newSD = (*varit)->clone();
                newSD->unionVar(sdr);
                indexVars.insert(varit, newSD.get());
                myVars.push_back(move(newSD));
                return true;
            }
            else
            {
                (*varit)->unionVar(sdr);
                ++it;
                ++varit;
            }
        }
        throw std::runtime_error("shouldn't get here");
    }

    void nondet(unsigned int i)
    {
        std::unique_ptr<SymbolicDouble> undet = std::make_unique<SymbolicDouble>("undet", reporter);
        undet->nondet();
        set(i, move(undet));
    }

    void nondet(SymbolicDouble* index)
    {
        SymbolicDouble undet("undet", reporter);
        undet.nondet();
        set(index, &undet);
    }

    bool unionArray(const SymbolicArray* other)
    {
        auto myIndexIt = indicies.begin();
        auto myVarsIt = indexVars.begin();
        auto theirIndexIt = other->indicies.cbegin();
        auto theirVarsIt = other->indexVars.cbegin();

        bool change = false;

        while (myIndexIt != indicies.end() && theirIndexIt != other->indicies.end())
        {
            if (*myIndexIt > *theirIndexIt)
            {
                std::unique_ptr<SymbolicDouble> newVar = (*myVarsIt)->clone();
                if (newVar->unionVar(*theirVarsIt))
                {
                    change = true;
                    myIndexIt = indicies.insert(myIndexIt, *theirIndexIt);
                    myVarsIt = indexVars.insert(myVarsIt, newVar.get());
                    myVars.push_back(move(newVar));
                    ++myIndexIt;
                    ++myVarsIt;
                }
                ++theirIndexIt;
                ++theirVarsIt;
            }
            else if (*myIndexIt == *theirIndexIt)
            {
                if ((*myVarsIt)->unionVar(*theirVarsIt)) change = true;
                ++myIndexIt;
                ++myVarsIt;
                ++theirIndexIt;
                ++theirVarsIt;
            }
            else
            {
                ++myIndexIt;
                ++myVarsIt;
            }
        }
    }

    std::string diagString()
    {
        int lastIndex = 0;
        auto indIt = indicies.begin();
        auto varIt = indexVars.begin();

        std::string out;
        while (indIt != indicies.end())
        {
            out += "[" + std::to_string(lastIndex) + ", " + std::to_string(*indIt) + "): ";
            out += std::to_string((*varIt)->getLowerBound()) + " - " + std::to_string((*varIt)->getUpperBound());
            out += "\n";

            lastIndex = *indIt;
            ++indIt;
            ++varIt;
        }
        return out;
    }
};

#endif //PROJECT_SYMBOLICARRAY_H
