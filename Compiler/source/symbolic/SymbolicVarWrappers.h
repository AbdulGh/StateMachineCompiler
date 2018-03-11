//
// Created by abdul on 06/02/18.
//

#ifndef PROJECT_SYMBOLICVARWRAPPERS_H
#define PROJECT_SYMBOLICVARWRAPPERS_H

#include "SymbolicExecution.h"

enum class AccessType{DIRECT, BYARRAY};

template <typename T>
class GottenVarPtr
{
private:
    union
    {
        T* rp;
        std::unique_ptr<T> up;
    };
    const bool owns;

public:
    GottenVarPtr(GottenVarPtr&& o) : owns(o.owns)
    {
        if (owns) up = std::move(o.up);
        else rp = o.rp;
    }

    /* debug GottenVarPtr& operator=(GottenVarPtr&& o) : owns(o.owns)
    {
        if (owns) up = std::move(o.up);
        else rp = o.rp;
        return *this;
    }*/

    explicit GottenVarPtr(T* pointTo) : rp(pointTo), owns(false) {}
    explicit GottenVarPtr(std::unique_ptr<T> sv) : up(std::move(sv)), owns(true) {}
    ~GottenVarPtr() {if (owns) up.reset();}

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

/*
std::unique_ptr<VarGetter> parseAccess(const std::string& toParse, AbstractCommand::StringType* st = nullptr)
{
    if (toParse.empty()) throw "Can't parse an empty string";
    if (toParse[0] == '\"')
    {
        *st = AbstractCommand::StringType::STRINGLIT;
        return nullptr;
    }
    else try
    {
        std::stod(toParse);
        *st = AbstractCommand::StringType::DOUBLELIT;
        return nullptr;
    }
    catch (std::invalid_argument&) //id or array access
    {
    }
}*/

class VarGetter
{
protected:
    AccessType accessType;
    void setAccessType(AccessType at) {accessType = at;}

public:

    const AccessType& getAccessType() {return accessType;}

    virtual GottenVarPtr<SymbolicVariable> getSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef) = 0;
    virtual GottenVarPtr<SymbolicDouble> getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) = 0;
    virtual bool check(SymbolicExecution::SymbolicExecutionFringe* sef) {return true;};
    virtual std::unique_ptr<VarGetter> clone() = 0;
    virtual VariableType getVariableType(SymbolicExecution::SymbolicExecutionFringe *sef) = 0;
    virtual std::string getName() const = 0;

    operator std::string() {return getName();}
};

class VarSetter
{
public:
    virtual void setSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicDouble *sv) = 0;
    virtual bool check(SymbolicExecution::SymbolicExecutionFringe* sef) {return true;};
};

class GetSVByName: public VarGetter
{
public:
    const std::string name;

    GetSVByName(std::string n) : name(move(n)) {VarGetter::setAccessType(AccessType::DIRECT);};

    GottenVarPtr<SymbolicVariable> getSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicVariable* foundsv = sef->symbolicVarSet->findVar(name);
        return GottenVarPtr<SymbolicVariable>(foundsv);
    }

    GottenVarPtr<SymbolicDouble> getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicVariable* sv = sef->symbolicVarSet->findVar(name);
        if (sv == nullptr) throw std::runtime_error("Var '" + name + "' undeclared");
        if (sv->getType() != DOUBLE) throw std::runtime_error("wrong type");
        SymbolicDouble* sd = static_cast<SymbolicDouble*>(sv);
        
        return std::move(GottenVarPtr<SymbolicDouble>(sd));
    }

    VariableType getVariableType(SymbolicExecution::SymbolicExecutionFringe *sef) override
    {
        SymbolicVariable* sv = sef->symbolicVarSet->findVar(name);
        if (sv == nullptr) throw std::runtime_error("Var '" + name + "' undeclared");
        return sv->getType();
    }
    
    std::string getName() const override 
    {
        return name;
    }

    std::unique_ptr<VarGetter> clone() override
    {
        return std::make_unique<GetSVByName>(name);
    }
};

class GetSDByArrayIndex: public VarGetter
{
public:
    const std::string name;
    unsigned int index;

    GetSDByArrayIndex(std::string n, unsigned int i): name(move(n)), index(i)

    {
        VarGetter::setAccessType(AccessType::DIRECT);
        if (i < 0) throw std::runtime_error("don't be silly!");
    };

    GottenVarPtr<SymbolicVariable> getSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        throw "bad!";
    }

    GottenVarPtr<SymbolicDouble> getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
        if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
        return std::move(GottenVarPtr<SymbolicDouble>(std::move(sa->operator[](index)))); //WHY operator[]???!
    }

    VariableType getVariableType(SymbolicExecution::SymbolicExecutionFringe *sef) override
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

    std::unique_ptr<VarGetter> clone() override
    {
        return std::make_unique<GetSDByArrayIndex>(name, index);
    }
};

class GetSDByIndexVar: public VarGetter
{
public:
    const std::string arrayName;
    std::unique_ptr<VarGetter> index;

    GetSDByIndexVar(std::string arrN, std::unique_ptr<VarGetter> var):
            arrayName(move(arrN)), index(std::move(var)) {VarGetter::setAccessType(AccessType::DIRECT);};

    GottenVarPtr<SymbolicVariable> getSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        throw "accesses array";
    }

    GottenVarPtr<SymbolicDouble> getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(arrayName);
        if (sa == nullptr) throw std::runtime_error("Array '" + arrayName + "' undeclared");
        auto sv = index->getSymbolicDouble(sef);
        if (sv->getType() != DOUBLE) throw std::runtime_error("wrong type");
        GottenVarPtr<SymbolicDouble> sd(sa->operator[](sv.get()));
        return std::move(sd);
    }

    VariableType getVariableType(SymbolicExecution::SymbolicExecutionFringe *sef) override
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

    std::unique_ptr<VarGetter> clone() override
    {
        return std::make_unique<GetSDByIndexVar>(arrayName, index->clone());
    }
};

class SetArraySDByIndex: public VarSetter
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

class SetArrayByVar: public VarSetter
{
public:
    const std::string arrayName;
    std::unique_ptr<VarGetter> index;

    SetArrayByVar(std::string arrN, std::unique_ptr<VarGetter> var):
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

class SetSVByName: public VarSetter
{
public:
    const std::string name;

    SetSVByName(std::string n) : name(move(n)) {};

    void setSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicDouble *sv) override
    {
        sv->setName(name);
        sef->symbolicVarSet->addVar(sv->clone());
    }
};
#endif //PROJECT_SYMBOLICVARWRAPPERS_H
