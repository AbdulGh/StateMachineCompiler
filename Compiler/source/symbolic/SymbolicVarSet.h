#ifndef PROJECT_SYMBOLICVARSET_H
#define PROJECT_SYMBOLICVARSET_H

#include <unordered_map>
#include <stack>
#include <memory>

#include "SymbolicDouble.h"
#include "SymbolicArray.h"


typedef std::unique_ptr<SymbolicDouble> SymbolicDoublePointer;
typedef std::unordered_map<std::string, SymbolicDoublePointer> VarMap;
typedef std::unique_ptr<SymbolicArray> SymbolicArrayPointer;
typedef std::unordered_map<std::string, SymbolicArrayPointer> ArrayMap;

class SymbolicVarSet;

class SVSIterator
{
private:
    const SymbolicVarSet* currentSVS;
    VarMap::const_iterator currentIt;

public:
    SVSIterator(const SymbolicVarSet* start, VarMap::const_iterator cit)
            : currentSVS(start), currentIt(cit) {}

    bool operator!=(const SVSIterator& other);
    const std::pair<const std::string, SymbolicDoublePointer>& operator*();
    SVSIterator& operator++();
};

class SymbolicVarSet
{
private:
    std::shared_ptr<SymbolicVarSet> parent = nullptr;
    VarMap::const_iterator endIterator;
    VarMap variables;
    ArrayMap arrays;

public:
    explicit SymbolicVarSet(std::shared_ptr<SymbolicVarSet> p)
    {
        if (p == nullptr) endIterator = variables.cend();
        else endIterator = p->endIterator;
        parent = move(p);
    }
    SymbolicVarSet(const SymbolicVarSet&) = delete;
    SymbolicDouble* findVar(std::string name);
    SymbolicArray* findArray(std::string name);
    //const std::unordered_map<std::string, SymbolicDoublePointer>& getVars() const {return variables;}
    void addVar(SymbolicDoublePointer newvar);
    void addArray(const std::string& name, SymbolicArrayPointer sap);
    bool unionSVS(SymbolicVarSet* other);
    bool isFeasable();

    void setLoopInit();

    std::vector<std::pair<const std::string, SymbolicDouble*>> getAllVars();
    std::vector<std::pair<const std::string, SymbolicArray*>> getAllArrays();

    SVSIterator begin() const {throw std::runtime_error("not working");}// return {this, variables.cbegin()};} (todo)
    SVSIterator end() const {return parent == nullptr ? SVSIterator(this, variables.cend()) : parent->end();} //end only called once when iterating

    friend class SVSIterator;
};


#endif
