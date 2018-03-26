//
// Created by abdul on 03/02/18.
//

#ifndef PROJECT_SYMBOLICARRAY_H
#define PROJECT_SYMBOLICARRAY_H

#include <list>
#include <map>

#include "SymbolicVariables.h"

//todo indeterminate size
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
            std::unique_ptr<SymbolicDouble> clone = otherVar->cloneSD();
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
            if (*it > n) return (*varit)->cloneSD();
            else
            {
                ++it;
                ++varit;
            }
        }
        throw "shouldn't happen";
    }

    std::unique_ptr<SymbolicDouble> operator[](SymbolicDouble* index)
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

        std::unique_ptr<SymbolicDouble> cp = (*varit)->cloneSD();

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

        throw "shouldn't get here";
    }

    bool set(unsigned int n, double v)
    {
        SymbolicDouble sd("constDouble", reporter);
        sd.setTConstValue(v);
        set(n, &sd); //todo this gets built and immediately cloned
    }

    bool set(unsigned int n, SymbolicDouble* sdr) //todo check if we need to clone sdr
    {
        std::unique_ptr<SymbolicDouble> sd = sdr->cloneSD();
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

    bool set(SymbolicDouble* index, SymbolicDouble* sdr)
    {
        if (index->isDetermined()) set(index->getTConstValue(), sdr);
        std::unique_ptr<SymbolicDouble> sd = sdr->cloneSD();
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

    void nondet(unsigned int i)
    {
        SymbolicDouble undet("undet", reporter);
        undet.nondet();
        set(i, &undet);
    }

    void nondet(SymbolicDouble* index)
    {
        SymbolicDouble undet("undet", reporter);
        undet.nondet();
        set(index, &undet);
    }
};

#endif //PROJECT_SYMBOLICARRAY_H
