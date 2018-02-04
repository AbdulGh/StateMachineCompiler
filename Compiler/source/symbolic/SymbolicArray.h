//
// Created by abdul on 03/02/18.
//

#ifndef PROJECT_SYMBOLICARRAY_H
#define PROJECT_SYMBOLICARRAY_H

#include <vector>

#include "SymbolicVariables.h"

class SymbolicArray
{
private: //todo forward lists
    std::vector<int> indicies;
    std::vector<std::unique_ptr<SymbolicDouble>> myVars;
    //<l_1, u_1, l_2, u_2...> where vars[0] is in indicies [l_1, u_1)
    std::vector<SymbolicDouble*> indexVars;
    const unsigned int size;
    Reporter& reporter;

public:
    SymbolicArray(unsigned int n, Reporter& r): size(n), reporter(r)
    {
        myVars.emplace_back(std::make_unique<SymbolicDouble>("uninit", reporter));
        indicies.push_back(size);
        indexVars.push_back(myVars.begin()->get());
    }
    
    const SymbolicDouble* operator[](unsigned int n)
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

    bool set(unsigned int n, double v)
    {
        std::unique_ptr<SymbolicDouble> sd = std::make_unique<SymbolicDouble>("constDouble", reporter);
        sd->setTConstValue(v);
        set(n, move(sd));
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
        throw "shouldnt reach here";
    }

    bool set(SymbolicDouble* index, std::unique_ptr<SymbolicDouble> sd)
    {
        if (index->isDetermined()) set(index->getTConstValue(), std::move(sd));
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
        std::unique_ptr<SymbolicDouble> cp = (*varit)->cloneSD();
        varit = indexVars.insert(++varit, cp.get());
        myVars.push_back(move(cp));

        while (it != indicies.end() && varit != indexVars.end())
        {
            if (*it == index->getTUpperBound() + 1) return true;
            else if (*it > index->getTUpperBound() + 1)
            {
                indicies.insert(it, index->getTUpperBound() + 1);
                std::unique_ptr<SymbolicDouble> newSD = (*varit)->cloneSD();
                newSD->unionVar(sd.get());
                indexVars.insert(varit, newSD.get());
                myVars.push_back(std::move((*varit)->cloneSD()));
                delet();
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
