#ifndef PROJECT_SYMBOLICVARSET_H
#define PROJECT_SYMBOLICVARSET_H

#include <unordered_map>
#include <stack>
#include <memory>

#include "SymbolicVariables.h"


typedef std::unique_ptr<SymbolicVariable> SymbolicVariablePointer;
typedef std::unordered_map<std::string, SymbolicVariablePointer> VarMap;

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
    const std::pair<const std::string, SymbolicVariablePointer>& operator*();
    SVSIterator& operator++();
};

class SymbolicVarSet
{
private:
    std::shared_ptr<SymbolicVarSet> parent = nullptr;
    VarMap::const_iterator endIterator;
    VarMap variables;

public:
    explicit SymbolicVarSet(std::shared_ptr<SymbolicVarSet> p)
    {
        if (p == nullptr) endIterator = variables.cend();
        else endIterator = p->endIterator;
        parent = move(p);
    }
    SymbolicVarSet(const SymbolicVarSet&) = delete;
    SymbolicVariable* findVar(std::string name);
    //const std::unordered_map<std::string, SymbolicVariablePointer>& getVars() const {return variables;}
    void defineVar(SymbolicVariablePointer newvar);
    void unionSVS(SymbolicVarSet* other);
    bool isFeasable();

    void setLoopInit();

    std::vector<std::pair<const std::string, SymbolicVariable*>> getAllVars();

    SVSIterator begin() const {throw "not working";}// return {this, variables.cbegin()};}
    SVSIterator end() const {return parent == nullptr ? SVSIterator(this, variables.cend()) : parent->end();} //end only called once when iterating

    friend class SVSIterator;
};


#endif
