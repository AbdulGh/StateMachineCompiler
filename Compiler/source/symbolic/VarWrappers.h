//
// Created by abdul on 06/02/18.
//

#ifndef PROJECT_VARWRAPPERS_H
#define PROJECT_VARWRAPPERS_H

#include <memory>
#include "../compile/Token.h"

class SymbolicVariable;
class SymbolicArray;
class SymbolicDouble;
namespace SymbolicExecution{class SymbolicExecutionFringe;};

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
    GottenVarPtr(GottenVarPtr& o) = delete;
    GottenVarPtr& operator=(GottenVarPtr<T> rhs) = delete;
    GottenVarPtr& operator=(GottenVarPtr<T>&& rhs) = delete;

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
        else return std::unique_ptr<T>(rp);
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
    virtual bool check(SymbolicExecution::SymbolicExecutionFringe* sef) const = 0;
    virtual std::unique_ptr<VarWrapper> clone() const  = 0;
    virtual VariableType getVariableType(SymbolicExecution::SymbolicExecutionFringe* sef) const = 0;
    virtual void setSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicVariable* sv) = 0;
    virtual void setConstValue(SymbolicExecution::SymbolicExecutionFringe* sef, std::string sv) = 0;
    virtual void nondet(SymbolicExecution::SymbolicExecutionFringe* sef) = 0;
};


class SVByName: public VarWrapper
{
public:
    SVByName(std::string n)
    {
        setName(move(n));
        setAccessType(AccessType::DIRECT);
    }

    GottenVarPtr<SymbolicVariable> getSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef) const override;
    GottenVarPtr<SymbolicDouble> getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) const override;
    VariableType getVariableType(SymbolicExecution::SymbolicExecutionFringe* sef) const override;
    void setSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicVariable* sv) override;
    void setConstValue(SymbolicExecution::SymbolicExecutionFringe* sef, std::string sv) override;
    void nondet(SymbolicExecution::SymbolicExecutionFringe* sef) override;
    bool check(SymbolicExecution::SymbolicExecutionFringe* sef) const override;
    std::string getFullName() const override;
    std::unique_ptr<VarWrapper> clone() const override;
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

    std::string getFullName() const override
    {
        return name + "[" + std::to_string(index) + "]";
    }

    VariableType getVariableType(SymbolicExecution::SymbolicExecutionFringe* sef) const override;
    GottenVarPtr<SymbolicDouble> getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) const override;
    bool check(SymbolicExecution::SymbolicExecutionFringe* sef) const override;
    void setSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicVariable* sv) override;
    void setConstValue(SymbolicExecution::SymbolicExecutionFringe* sef, std::string sv) override;
    void nondet(SymbolicExecution::SymbolicExecutionFringe* sef) override;
    std::unique_ptr<VarWrapper> clone() const override;
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

    std::string getFullName() const override
    {
        return name + "[" + index->getFullName() + "]";
    }

    GottenVarPtr<SymbolicDouble> getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) const override;
    VariableType getVariableType(SymbolicExecution::SymbolicExecutionFringe* sef) const override;
    bool check(SymbolicExecution::SymbolicExecutionFringe* sef) const override;
    void setSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicVariable* sv) override;
    void setConstValue(SymbolicExecution::SymbolicExecutionFringe* sef, std::string sv) override;
    void nondet(SymbolicExecution::SymbolicExecutionFringe* sef) override;
    std::unique_ptr<VarWrapper> clone() const override;
};

#endif
