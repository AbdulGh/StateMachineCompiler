//
// Created by abdul on 06/02/18.
//

#ifndef PROJECT_SYMBOLICVARWRAPPERS_H
#define PROJECT_SYMBOLICVARWRAPPERS_H

#include "SymbolicExecution.h"

class SymbolicDoubleGetter
{
public:
    virtual std::shared_ptr<SymbolicDouble> getSD(SymbolicExecution::SymbolicExecutionFringe* sef) = 0;
    virtual bool check(SymbolicExecution::SymbolicExecutionFringe* sef) {return true;};
    virtual std::string getName() const = 0;
};

class SymbolicDoubleSetter
{
public:
    virtual void setSD(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicDouble* sv) = 0;
    virtual bool check(SymbolicExecution::SymbolicExecutionFringe* sef) {return true;};
};

class GetSDByName: public SymbolicDoubleGetter
{
public:
    const std::string name;

    GetSDByName(std::string n) : name(move(n)) {};

    std::shared_ptr<SymbolicDouble> getSD(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicVariable* sv = sef->symbolicVarSet->findVar(name);
        if (sv == nullptr) throw std::runtime_error("Var '" + name + "' undeclared");
        if (sv->getType() != DOUBLE) throw std::runtime_error("wrong type");
        SymbolicDouble* sd = static_cast<SymbolicDouble*>(sv);
        
        return std::shared_ptr<SymbolicDouble>(sd);
    }
    
    std::string getName() const override 
    {
        return name;
    }
};

class GetSDByArrayIndex: public SymbolicDoubleGetter
{
public:
    const std::string name;
    unsigned int index;

    GetSDByArrayIndex(const std::string& n, unsigned int& i): name(n), index(i)
        {if (i < 0) throw std::runtime_error("don't be silly!");};

    std::shared_ptr<SymbolicDouble> getSD(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
        if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
        //here!
    }
    
    bool check(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
        if (sa == nullptr)
        {
            sef->error(Reporter::UNDECLARED_USE, "Array '" + name + "' undeclared");
            return false;
        }
        sa->checkIndex(index);
    }
    
    std::string getName() const override 
    {
        return name + "[" + std::to_string(index) + "]";
    }
};

class GetSDByIndexVar: public SymbolicDoubleGetter
{
public:
    const std::string arrayName;
    std::unique_ptr<SymbolicDoubleGetter> index;

    GetSDByIndexVar(const std::string& arrN, std::unique_ptr<SymbolicDoubleGetter> var): arrayName(arrN), index(std::move(var)) {}

    std::shared_ptr<SymbolicDouble> getSD(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(arrayName);
        if (sa == nullptr) throw std::runtime_error("Array '" + arrayName + "' undeclared");
        auto sv = index->getSD(sef);
        if (sv->getType() != DOUBLE) throw std::runtime_error("wrong type");
        return std::move(sa[sv.get()]);
    }

    bool check(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(arrayName);
        if (sa == nullptr)
        {
            sef->error(Reporter::UNDECLARED_USE, "Array '" + arrayName + "' undeclared");
            return false;
        }

        SymbolicDouble* sv = index->getSD(sef).get();
        if (sv->isDetermined()) return sa->checkIndex(sv->getTConstValue());
        else return sa->checkBounds(sv->getTLowerBound(), sv->getTUpperBound());
    }
    
    std::string getName() const override
    {
        return arrayName + "[" + index->getName() + "]";
    }
};

class SetArraySDByIndex: public SymbolicDoubleSetter
{
public:
    const std::string name;
    const unsigned long index;

    SetArraySDByIndex(const std::string& n, const unsigned long& i): name(n), index(i) {};

    void setSD(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicDouble* sv) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
        if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
        sa->set(index, sv);
    }

    bool check(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
        if (sa == nullptr)
        {
            sef->error(Reporter::UNDECLARED_USE, "Array '" + name + "' undeclared");
            return false;
        }
        sa->checkIndex(index);
    }
};

class SetArraySDByVar: public SymbolicDoubleSetter
{
public:
    const std::string arrayName;
    SymbolicDoubleGetter index;

    SetArraySDByVar(const std::string& arrN, SymbolicDoubleGetter var): arrayName(arrN), index(std::move(var)) {}

    void setSD(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicDouble* sv) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(arrayName);
        if (sa == nullptr) throw std::runtime_error("Array '" + arrayName + "' undeclared");
        SymbolicDouble* ind = index.getSD(sef).get();
        sa->set(ind, sv);
        
    }

    bool check(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(arrayName);
        if (sa == nullptr)
        {
            sef->error(Reporter::UNDECLARED_USE, "Array '" + arrayName + "' undeclared");
            return false;
        }

        SymbolicDouble* sv = index.getSD(sef).get();
        if (sv->isDetermined()) return sa->checkIndex(sv->getTConstValue());
        else return sa->checkBounds(sv->getTLowerBound(), sv->getTUpperBound());
    }
};

class SetSDByName: public SymbolicDoubleSetter
{
public:
    const std::string name;

    SetSDByName(const std::string& n) : name(n) {};

    void setSD(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicDouble* sv) override
    {
        sv->setName(name);
        sef->symbolicVarSet->addVar(std::move(sv));
    }
};
#endif //PROJECT_SYMBOLICVARWRAPPERS_H
