//
// Created by abdul on 03/02/18.
//

#ifndef PROJECT_SYMBOLICARRAY_H
#define PROJECT_SYMBOLICARRAY_H

#include <list>
#include <map>

#include "SymbolicVariables.h"

//todo add name
//todo indeterminate size
class SymbolicArray
{
private:
    //<u_0, u_1... > where vars[i] is in indicies [u_{i-1}, u_i)
    std::list<int> indicies;
    std::list<std::shared_ptr<SymbolicDouble>> indexVars;
    const unsigned int size;
    Reporter& reporter;
    std::string name;

public:
    SymbolicArray(std::string id, unsigned int n, Reporter& r): size(n), reporter(r), name(std::move(id))
    {
        indicies.push_back(size);
        indexVars.push_back(std::make_shared<SymbolicDouble>("uninit", reporter));
    }

    SymbolicArray(const SymbolicArray& other): reporter(other.reporter), size(other.size)
    {
        indicies = other.indicies;
        for (auto& ptr : other.indexVars)
        {
            indexVars.push_back(ptr->cloneSP());
        }
    }

    std::unique_ptr<SymbolicArray> clone()
    {
        return std::make_unique<SymbolicArray>(*this);
    }
    
    bool checkIndex(long index)
    {
        if (index < 0) reporter.error(Reporter::ARRAY_BOUNDS, "Asked for negative index from array");
        else if (index >= size) reporter.error(Reporter::ARRAY_BOUNDS,
                                               "Asked to get index " + std::to_string(index) + " in array of size " + std::to_string(size));
    }
    
    bool checkBounds(double lbd, double ubd, int& lb, int& ub)
    {
        lb = ceil(lbd);
        ub = floor(ubd);

        if (ub < 0)
        {
            reporter.error(Reporter::ARRAY_BOUNDS,
                           "Asked to get index <= " + std::to_string(ub) + " in array");
            return false;
        }
        else if (lb >= size)
        {
            reporter.error(Reporter::ARRAY_BOUNDS,
                           "Asked to get index >= " + std::to_string(lb) + " in array of size " + std::to_string(size));
            return false;
        }

        if (lb < 0)
        {
            reporter.warn(Reporter::ARRAY_BOUNDS, "Asked to access possibly negative index");
            lb = 0;
        }
        else if (ub >= size)
        {
            reporter.warn(Reporter::ARRAY_BOUNDS,
                          "Could access index up to " + std::to_string(ub) + " in index of size " + std::to_string(size));
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
        else if (lbd >= size)
        {
            reporter.error(Reporter::ARRAY_BOUNDS,
                           "Asked to get index >= " + std::to_string(lbd) + " in array of size " + std::to_string(size));
            return false;
        }

        if (lbd < 0) reporter.warn(Reporter::ARRAY_BOUNDS, "Asked to access possibly negative index");

        else if (ubd >= size)
        {
            reporter.warn(Reporter::ARRAY_BOUNDS,
                          "Could access index up to " + std::to_string(ubd) + " in index of size " + std::to_string(size));
        }

        return true;
    }

    std::shared_ptr<SymbolicDouble> operator[](unsigned int n)
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
            if (*it > n) return *varit;
            else
            {
                ++it;
                ++varit;
            }
        }
        throw "shouldn't happen";
    }

    std::shared_ptr<SymbolicDouble> operator[](SymbolicDouble* index)
    {
        int lb, ub;

        if (!checkBounds(index->getTLowerBound(), index->getTUpperBound(), lb, ub)) return nullptr;

        auto it = indicies.begin();
        auto varit = indexVars.begin();

        while (*it <= lb)
        {
            ++it;
            ++varit;
            if (it == indicies.end() || varit == indexVars.end()) throw "bad";
        }

        std::shared_ptr<SymbolicDouble> cp = (*varit)->cloneSP();

        while (it != indicies.end() || varit != indexVars.end())
        {
            if (ub >= *it)
            {
                ++varit;
                ++it;
                cp->unionVar(varit->get());
            }
            else return std::move(cp);
        }

        throw "shouldn't get here";
    }

    bool set(unsigned int n, double v)
    {
        SymbolicDouble sd("constDouble", reporter);
        sd.setTConstValue(v);
        set(n, &sd); //todo this gets built and immediately cloned
    }

    bool set(unsigned int n, SymbolicDouble* sdr)
    {
        std::shared_ptr<SymbolicDouble> sd = sdr->cloneSP();
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
                indexVars.insert(varit, move(sd));
                return true;
            }
            else
            {
                ++it;
                ++varit;
            }
        }
        throw "shouldnt reach here";
    }

    bool set(SymbolicDouble* index, SymbolicDouble* sdr)
    {
        if (index->isDetermined()) set(index->getTConstValue(), sdr);
        std::shared_ptr<SymbolicDouble> sd = sdr->cloneSP();
        if (index->getTUpperBound() < 0)
        {
            reporter.error(Reporter::ARRAY_BOUNDS,
                           "Asked to set index <= " + std::to_string(index->getTUpperBound()) + " in array");
            return false;
        }
        else if (index->getTLowerBound() >= size)
        {
            reporter.error(Reporter::ARRAY_BOUNDS,
                           "Asked to set index >= " + std::to_string(index->getTLowerBound()) + " in array of size " + std::to_string(size));
            return false;
        }

        if (index->getTLowerBound() < 0) reporter.warn(Reporter::ARRAY_BOUNDS, "Asked to access possibly negative index");
        else if (index->getTUpperBound() >= size)
        {
            reporter.warn(Reporter::ARRAY_BOUNDS,
                          "Could access index up to " + std::to_string(index->getTUpperBound()) + " in index of size " + std::to_string(size));
        }

        auto it = indicies.begin();
        auto varit = indexVars.begin();

        while (*it <= index->getTLowerBound())
        {
            ++it;
            ++varit;
            if (it == indicies.end() || varit == indexVars.end()) throw "bad";
        }

        it = indicies.insert(it, index->getTLowerBound());
        ++it;
        varit = indexVars.insert(++varit, (*varit)->cloneSP());

        while (it != indicies.end() && varit != indexVars.end())
        {
            if (*it == index->getTUpperBound() + 1) return true;
            else if (*it > index->getTUpperBound() + 1)
            {
                indicies.insert(it, index->getTUpperBound() + 1);
                std::shared_ptr<SymbolicDouble> newSD = (*varit)->cloneSP();
                newSD->unionVar(sd.get());
                indexVars.insert(varit, newSD);
                return true;
            }
            else
            {
                (*varit)->unionVar(sd.get());
                ++it;
                ++varit;
            }
        }
        throw "shouldn't get here";
    }
};


#endif //PROJECT_SYMBOLICARRAY_H
