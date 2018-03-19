//
// Created by abdul on 06/02/18.
//

#ifndef PROJECT_VARWRAPPERS_H
#define PROJECT_VARWRAPPERS_H

//#include "../symbolic/SymbolicExecution.h"
//#include "../symbolic/SymbolicArray.h"

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

    std::unique_ptr<T> release()
    {
        if (constructed()) return move(up);
        else
        {
            return std::unique_ptr<T>(rp);
        }
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
    virtual GottenVarPtr<SymbolicVariable> getSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef) const = 0;
    virtual GottenVarPtr<SymbolicDouble> getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) const  = 0;
    virtual bool check(SymbolicExecution::SymbolicExecutionFringe* sef) const {return true;};
    virtual std::unique_ptr<VarWrapper> clone() const  = 0;
    virtual VariableType getVariableType(SymbolicExecution::SymbolicExecutionFringe* sef) const = 0;
    virtual void setSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicVariable* sv) = 0;
    virtual void setConstValue(SymbolicExecution::SymbolicExecutionFringe* sef, std::string sv) = 0;
    virtual void nondet(SymbolicExecution::SymbolicExecutionFringe* sef) = 0;
    virtual bool check(SymbolicExecution::SymbolicExecutionFringe* sef) {return true;}
};


class SVByName: public VarWrapper
{
public:
    SVByName(std::string n)
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

    VariableType getVariableType(SymbolicExecution::SymbolicExecutionFringe* sef) const override
    {
        SymbolicVariable* sv = sef->symbolicVarSet->findVar(name);
        if (sv == nullptr) throw std::runtime_error("Var '" + name + "' undeclared");
        return sv->getType();
    }

    void setSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicVariable* sv) override
    {
        sv->setName(name);
        sef->symbolicVarSet->addVar(sv->clone());
    }

    void setConstValue(SymbolicExecution::SymbolicExecutionFringe* sef, std::string sv)
    {
        auto var = sef->symbolicVarSet->findVar(name);
        if (!var) throw std::runtime_error("Undefined variable '" + name + "'");
        var->setConstValue(sv);
    }

    void nondet(SymbolicExecution::SymbolicExecutionFringe* sef)
    {
        auto var = sef->symbolicVarSet->findVar(name);
        if (!var) throw std::runtime_error("Undefined variable '" + name + "'");
        var->userInput();
    }

    std::string getFullName() const override
    {
        return name;
    }

    std::unique_ptr<VarWrapper> clone() const override
    {
        return std::make_unique<SVByName>(name);
    }
};

class SDByArrayIndex: public VarWrapper
{
public:
    unsigned int index;

    SDByArrayIndex(std::string n, unsigned int i): index(i)
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

    VariableType getVariableType(SymbolicExecution::SymbolicExecutionFringe *sef) const override
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
    
    std::string getFullName() const override
    {
        return name + "[" + std::to_string(index) + "]";
    }

    std::unique_ptr<VarWrapper> clone() const override
    {
        return std::make_unique<SDByArrayIndex>(name, index);
    }
};

class SDByIndexVar: public VarWrapper
{
public:
    std::unique_ptr<VarWrapper> index;

    SDByIndexVar(std::string arrN, std::unique_ptr<VarWrapper> var):
            index(move(var))
    {
        setName(move(arrN));
        setAccessType(AccessType::DIRECT);
    }

    GottenVarPtr<SymbolicVariable> getSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef) const override
    {
        throw "accesses array";
    }

    GottenVarPtr<SymbolicDouble> getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) const override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
        if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
        auto sv = index->getSymbolicDouble(sef);
        if (sv->getType() != DOUBLE) throw std::runtime_error("wrong type");
        return GottenVarPtr<SymbolicDouble>(sa->operator[](sv.get()));
    }

    VariableType getVariableType(SymbolicExecution::SymbolicExecutionFringe *sef) const override
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

    std::string getFullName() const override
    {
        return name + "[" + index->getFullName() + "]";
    }

    std::unique_ptr<VarWrapper> clone() const override
    {
        return std::make_unique<SDByIndexVar>(name, index->clone());
    }
};

#endif
