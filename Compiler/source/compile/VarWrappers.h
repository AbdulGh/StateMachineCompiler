//
// Created by abdul on 06/02/18.
//

#ifndef PROJECT_SYMBOLICVARWRAPPERS_H
#define PROJECT_SYMBOLICVARWRAPPERS_H

#include "../symbolic/SymbolicExecution.h"

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
    bool owns;

public:
    GottenVarPtr(GottenVarPtr&& o) : owns(o.owns)
    {
        if (owns) up = move(o.up);
        else rp = o.rp;
    }
    explicit GottenVarPtr(T* pointTo) : rp(pointTo), owns(false) {}
    explicit GottenVarPtr(std::unique_ptr<T> sv) : up(move(sv)), owns(true) {}
    ~GottenVarPtr() {if (owns) up.reset();}

    void reset(T* pointTo)
    {
        if (owns) up.reset();
        rp = pointTo;
        owns = false;
    }

    void reset(std::unique_ptr<T> pointTo)
    {
        if (owns) up.reset();
        up = std::move(pointTo);
        owns = true;
    }

    void become(GottenVarPtr& o) //keep assignment operator deleted, I guess?
    {
        owns = o.owns;
        if (owns) up = move(o.up);
        else rp = o.rp;
    }

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

    explicit operator bool() const
    {
        return owns ? up != nullptr : rp != nullptr;
    }

    bool constructed() const
    {
        return !owns;
    }
};

class VarWrapper
{
protected:
    AccessType accessType;
    void setAccessType(AccessType at) {accessType = at;}
    std::string name;
    void setName(std::string n) {name = move(n);}
    
public:
    VarWrapper(std::string n): name(move(n)), accessType(AccessType::DIRECT) {}
    VarWrapper() = default;
    virtual std::string getFullName() const {return name;}
    virtual const std::string& getBaseName() const {return name;}
    const AccessType& getAccessType() {return accessType;}
};

class VarGetter: public VarWrapper
{
public:
    virtual GottenVarPtr<SymbolicVariable> getSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef) const = 0;
    virtual GottenVarPtr<SymbolicDouble> getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) const  = 0;
    virtual bool check(SymbolicExecution::SymbolicExecutionFringe* sef) const {return true;};
    virtual std::unique_ptr<VarGetter> clone() = 0;
    virtual VariableType getVariableType(SymbolicExecution::SymbolicExecutionFringe* sef) = 0;
};

class GetSVByName: public VarGetter
{
public:
    GetSVByName(std::string n)
    {
        setName(move(n));
        setAccessType(AccessType::DIRECT);
    }

    GottenVarPtr<SymbolicVariable> getSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef) const override
    {
        SymbolicVariable* foundsv = sef->symbolicVarSet->findVar(name);
        return GottenVarPtr<SymbolicVariable>(foundsv);
    }

    GottenVarPtr<SymbolicDouble> getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) const override
    {
        SymbolicVariable* sv = sef->symbolicVarSet->findVar(name);
        if (sv == nullptr) throw std::runtime_error("Var '" + name + "' undeclared");
        if (sv->getType() != DOUBLE) throw std::runtime_error("wrong type");
        SymbolicDouble* sd = static_cast<SymbolicDouble*>(sv);
        
        return GottenVarPtr<SymbolicDouble>(sd);
    }

    VariableType getVariableType(SymbolicExecution::SymbolicExecutionFringe *sef) const override
    {
        SymbolicVariable* sv = sef->symbolicVarSet->findVar(name);
        if (sv == nullptr) throw std::runtime_error("Var '" + name + "' undeclared");
        return sv->getType();
    }
    
    std::string getFullName() const override
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
    unsigned int index;

    GetSDByArrayIndex(std::string n, unsigned int i): index(i)
    {
        setAccessType(AccessType::DIRECT);
        setName(move(n));
    };

    GottenVarPtr<SymbolicVariable> getSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef) const override
    {
        throw "bad!";
    }

    GottenVarPtr<SymbolicDouble> getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) const override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
        if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
        return GottenVarPtr<SymbolicDouble>(move(sa->operator[](index))); //WHY operator[]???!
    }

    VariableType getVariableType(SymbolicExecution::SymbolicExecutionFringe *sef) override
    {
        return DOUBLE;
    }
    
    bool check(SymbolicExecution::SymbolicExecutionFringe* sef) const override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
        if (sa == nullptr)
        {
            sef->error(Reporter::UNDECLARED_USE, "Array '" + name + "' undeclared");
            return false;
        }
        sa->checkIndex(index);
    }
    
    std::string getFullName() const override
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
    std::unique_ptr<VarGetter> index;

    GetSDByIndexVar(std::string arrN, std::unique_ptr<VarGetter> var):
            index(move(var))
    {
        setName(move(arrN));
        setAccessType(AccessType::DIRECT);
    }

    GottenVarPtr<SymbolicVariable> getSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        throw "accesses array";
    }

    GottenVarPtr<SymbolicDouble> getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
        if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
        auto sv = index->getSymbolicDouble(sef);
        if (sv->getType() != DOUBLE) throw std::runtime_error("wrong type");
        return GottenVarPtr<SymbolicDouble>(sa->operator[](sv.get()));
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

        auto sv = index->getSymbolicDouble(sef);
        if (sv->isDetermined()) return sa->checkIndex(sv->getTConstValue());
        else return sa->checkBounds(sv->getTLowerBound(), sv->getTUpperBound());
    }

    std::string getFullName() const override
    {
        return name + "[" + index->getFullName() + "]";
    }

    std::unique_ptr<VarGetter> clone() override
    {
        return std::make_unique<GetSDByIndexVar>(name, index->clone());
    }
};

class VarSetter: public VarWrapper //todo have vtype set in ctor
{
public:
    virtual void setSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicVariable* sv) = 0;
    virtual void setConstValue(SymbolicExecution::SymbolicExecutionFringe* sef, std::string sv) = 0;
    virtual void nondet(SymbolicExecution::SymbolicExecutionFringe* sef) = 0;
    virtual bool check(SymbolicExecution::SymbolicExecutionFringe* sef) {return true;}
    virtual std::unique_ptr<VarSetter> clone() = 0;

};

class SetArraySDByIndex: public VarSetter
{
public:
    const unsigned long index;

    SetArraySDByIndex(std::string n, const unsigned long& i): index(i) {setName(move(n));}

    void setSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicVariable* sv) override
    {
        if (sv->getType() != DOUBLE) throw "should be double";
        SymbolicDouble* sd = static_cast<SymbolicDouble*>(sv);
        SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
        if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
        sa->set(index, sd);
    }

    void setConstValue(SymbolicExecution::SymbolicExecutionFringe* sef, std::string sv) override
    {
        double d;
        try {d = std::stod(sv);}
        catch (std::invalid_argument&) {throw std::runtime_error("Arrays hold only doubles");}
        SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
        if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
        sa->set(index, d);
    }

    void nondet(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
        if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
        sa->nondet(index);
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

    std::string getFullName() const override
    {
        return name + "[" + std::to_string(index) + "]";
    }

    std::unique_ptr<VarSetter> clone() {return std::make_unique<SetArraySDByIndex>(name, index);}
};

class SetArrayByVar: public VarSetter
{
public:
    std::unique_ptr<VarGetter> index;

    SetArrayByVar(std::string arrN, std::unique_ptr<VarGetter> var):
            index(move(var)) {setName(move(arrN));}

    void setSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicVariable* sv) override
    {
        if (sv->getType() != DOUBLE) throw "should be double";
        SymbolicDouble* sd = static_cast<SymbolicDouble*>(sv);
        SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
        if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
        SymbolicDouble* ind = index->getSymbolicDouble(sef).get();
        sa->set(ind, sd);
    }

    void setConstValue(SymbolicExecution::SymbolicExecutionFringe* sef, std::string sv) override
    {
        double d;
        try {d = std::stod(sv);}
        catch (std::invalid_argument&) {throw std::runtime_error("Arrays hold only doubles");}
        SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
        if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
        SymbolicDouble val("val", sef->reporter);
        val.setTConstValue(d);
        sa->set(index->getSymbolicDouble(sef).get(), &val);
    }


    void nondet(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
        if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
        sa->nondet(index->getSymbolicDouble(sef).get());
    }

    bool check(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
        if (sa == nullptr)
        {
            sef->error(Reporter::UNDECLARED_USE, "Array '" + name + "' undeclared");
            return false;
        }

        auto sv = index->getSymbolicDouble(sef);
        if (sv->isDetermined()) return sa->checkIndex(sv->getTConstValue());
        else return sa->checkBounds(sv->getTLowerBound(), sv->getTUpperBound());
    }

    std::string getFullName() const override
    {
        return name + "[" + index->getFullName() + "]";
    }

    std::unique_ptr<VarSetter> clone() override {return std::make_unique<SetArrayByVar>(name, index->clone());}

};

class SetSVByName: public VarSetter
{
public:
    SetSVByName(std::string n) {setName(move(n));};

    void setSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicVariable* sv) override
    {
        std::unique_ptr<SymbolicVariable> svc = sv->clone();
        svc->setName(name);
        sef->symbolicVarSet->addVar(move(svc));
    }

    void setConstValue(SymbolicExecution::SymbolicExecutionFringe* sef, std::string s) override
    {
        SymbolicVariable* sv = sef->symbolicVarSet->findVar(name);
        if (!sv) throw std::runtime_error("Undefined variable '" + name +  "'");
        sv->setConstValue(s);
    }

    std::string getFullName() const override
    {
        return name;
    }

    std::unique_ptr<VarSetter> clone() {return std::make_unique<SetSVByName>(name);}
};

#endif //PROJECT_SYMBOLICVARWRAPPERS_H
