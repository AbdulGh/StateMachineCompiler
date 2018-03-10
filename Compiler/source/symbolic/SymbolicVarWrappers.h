//
// Created by abdul on 06/02/18.
//

#ifndef PROJECT_SYMBOLICVARWRAPPERS_H
#define PROJECT_SYMBOLICVARWRAPPERS_H

#include "SymbolicExecution.h"

template <typename T>
class GottenSymVarPtr
{
private:
    union
    {
        T* rp;
        std::unique_ptr<T> up;
    };
    const bool owns;

public:
    GottenSymVarPtr(GottenSymVarPtr&& o) : owns(o.owns)
    {
        if (owns) up = std::move(o.up);
        else rp = o.rp;
    }

    /* debug GottenSymVarPtr& operator=(GottenSymVarPtr&& o) : owns(o.owns)
    {
        if (owns) up = std::move(o.up);
        else rp = o.rp;
        return *this;
    }*/

    explicit GottenSymVarPtr(T* pointTo) : rp(pointTo), owns(false) {}
    explicit GottenSymVarPtr(std::unique_ptr<T> sv) : up(std::move(sv)), owns(true) {}
    ~GottenSymVarPtr() {if (owns) up.reset();}

    T* get()
    {
        if (owns) return up.get();
        else return rp;
    }

    T* operator->()
    {
        return get();
    }

    T& operator*()
    {
        if (owns) return *up.get();
        else return *rp;
    }
};

class SymbolicVarGetter
{
public:
    virtual GottenSymVarPtr<SymbolicVariable> getSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef) = 0;
    virtual GottenSymVarPtr<SymbolicDouble> getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) = 0;
    virtual bool check(SymbolicExecution::SymbolicExecutionFringe* sef) {return true;};
    virtual std::unique_ptr<SymbolicVarGetter> clone() = 0;
    virtual VariableType getType(SymbolicExecution::SymbolicExecutionFringe* sef) = 0;
    virtual std::string getName() const = 0;
};

class SymbolicVarSetter
{
public:
    virtual void setSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicDouble *sv) = 0;
    virtual bool check(SymbolicExecution::SymbolicExecutionFringe* sef) {return true;};
};

class GetSVByName: public SymbolicVarGetter
{
public:
    const std::string name;

    GetSVByName(std::string n) : name(move(n)) {};

    GottenSymVarPtr<SymbolicVariable> getSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicVariable* foundsv = sef->symbolicVarSet->findVar(name);
        GottenSymVarPtr<SymbolicVariable> FUCK(foundsv);
        return FUCK;
    }

    GottenSymVarPtr<SymbolicDouble> getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicVariable* sv = sef->symbolicVarSet->findVar(name);
        if (sv == nullptr) throw std::runtime_error("Var '" + name + "' undeclared");
        if (sv->getType() != DOUBLE) throw std::runtime_error("wrong type");
        SymbolicDouble* sd = static_cast<SymbolicDouble*>(sv);
        
        return std::move(GottenSymVarPtr<SymbolicDouble>(sd));
    }

    VariableType getType(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicVariable* sv = sef->symbolicVarSet->findVar(name);
        if (sv == nullptr) throw std::runtime_error("Var '" + name + "' undeclared");
        return sv->getType();
    }
    
    std::string getName() const override 
    {
        return name;
    }

    std::unique_ptr<SymbolicVarGetter> clone() override
    {
        return std::make_unique<GetSVByName>(name);
    }
};

class GetSDByArrayIndex: public SymbolicVarGetter
{
public:
    const std::string name;
    unsigned int index;

    GetSDByArrayIndex(std::string n, unsigned int i): name(move(n)), index(i)
        {if (i < 0) throw std::runtime_error("don't be silly!");};

    GottenSymVarPtr<SymbolicVariable> getSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        throw "bad!";
    }

    GottenSymVarPtr<SymbolicDouble> getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
        if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
        return std::move(GottenSymVarPtr<SymbolicDouble>(std::move(sa->operator[](index)))); //WHY operator[]???!
    }

    VariableType getType(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        return DOUBLE;
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

    std::unique_ptr<SymbolicVarGetter> clone() override
    {
        return std::make_unique<GetSDByArrayIndex>(name, index);
    }
};

class GetSDByIndexVar: public SymbolicVarGetter
{
public:
    const std::string arrayName;
    std::unique_ptr<SymbolicVarGetter> index;

    GetSDByIndexVar(std::string arrN, std::unique_ptr<SymbolicVarGetter> var): arrayName(move(arrN)), index(std::move(var)) {}

    GottenSymVarPtr<SymbolicVariable> getSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        throw "accesses array";
    }

    GottenSymVarPtr<SymbolicDouble> getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(arrayName);
        if (sa == nullptr) throw std::runtime_error("Array '" + arrayName + "' undeclared");
        auto sv = index->getSymbolicDouble(sef);
        if (sv->getType() != DOUBLE) throw std::runtime_error("wrong type");
        GottenSymVarPtr<SymbolicDouble> sd(sa->operator[](sv.get()));
        return std::move(sd);
    }

    VariableType getType(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        return DOUBLE;
    }

    bool check(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(arrayName);
        if (sa == nullptr)
        {
            sef->error(Reporter::UNDECLARED_USE, "Array '" + arrayName + "' undeclared");
            return false;
        }

        auto sv = index->getSymbolicDouble(sef);
        if (sv->isDetermined()) return sa->checkIndex(sv->getTConstValue());
        else return sa->checkBounds(sv->getTLowerBound(), sv->getTUpperBound());
    }
    
    std::string getName() const override
    {
        return arrayName + "[" + index->getName() + "]";
    }

    std::unique_ptr<SymbolicVarGetter> clone() override
    {
        return std::make_unique<GetSDByIndexVar>(arrayName, index->clone());
    }
};

class SetArraySDByIndex: public SymbolicVarSetter
{
public:
    const std::string name;
    const unsigned long index;

    SetArraySDByIndex(std::string n, const unsigned long& i): name(move(n)), index(i) {};

    void setSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicDouble *sv) override
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

class SetArraySDByVar: public SymbolicVarSetter
{
public:
    const std::string arrayName;
    std::unique_ptr<SymbolicVarGetter> index;

    SetArraySDByVar(std::string arrN, std::unique_ptr<SymbolicVarGetter> var):
            arrayName(std::move(arrN)), index(std::move(var)) {}

    void setSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicDouble *sv) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(arrayName);
        if (sa == nullptr) throw std::runtime_error("Array '" + arrayName + "' undeclared");
        SymbolicDouble* ind = index->getSymbolicDouble(sef).get();
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

        auto sv = index->getSymbolicDouble(sef);
        if (sv->isDetermined()) return sa->checkIndex(sv->getTConstValue());
        else return sa->checkBounds(sv->getTLowerBound(), sv->getTUpperBound());
    }
};

class SetSDByName: public SymbolicVarSetter
{
public:
    const std::string name;

    SetSDByName(std::string n) : name(move(n)) {};

    void setSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicDouble *sv) override
    {
        sv->setName(name);
        sef->symbolicVarSet->addVar(sv->clone());
    }
};
#endif //PROJECT_SYMBOLICVARWRAPPERS_H
