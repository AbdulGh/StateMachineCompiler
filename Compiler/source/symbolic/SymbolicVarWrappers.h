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

class VarWrapper
{
protected:
    AccessType accessType;
    void setAccessType(AccessType at) {accessType = at;}
    std::string name;
    void setName(std::string n) {name = move(n);}
    
public:
    virtual std::string getFullName() const = 0;
    const std::string& getBaseName() {return name;}
    virtual std::string getUniqueID(SymbolTable& st) const = 0;
    const AccessType& getAccessType() {return accessType;}
};

class VarGetter: public VarWrapper
{
public:
    virtual GottenVarPtr<SymbolicVariable> getSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef) = 0;
    virtual GottenVarPtr<SymbolicDouble> getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) = 0;
    virtual bool check(SymbolicExecution::SymbolicExecutionFringe* sef) {return true;};
    virtual std::unique_ptr<VarGetter> clone() = 0;
    virtual VariableType getVariableType(SymbolicExecution::SymbolicExecutionFringe *sef) = 0;
};

class VarSetter: public VarWrapper
{
public:
    virtual void setSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicDouble *sv) = 0;
    virtual bool check(SymbolicExecution::SymbolicExecutionFringe* sef) {return true;};
};

class GetSVByName: public VarGetter
{
public:
    GetSVByName(std::string n)
    {
        setName(move(n));
        setAccessType(AccessType::DIRECT);
    }

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
        
        return GottenVarPtr<SymbolicDouble>(sd);
    }

    VariableType getVariableType(SymbolicExecution::SymbolicExecutionFringe *sef) override
    {
        SymbolicVariable* sv = sef->symbolicVarSet->findVar(name);
        if (sv == nullptr) throw std::runtime_error("Var '" + name + "' undeclared");
        return sv->getType();
    }
    
    std::string getFullName() const override
    {
        return name;
    }

    std::string getUniqueID(SymbolTable& st) const override
    {
        return st.findIdentifier(name)->getUniqueID();
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

    GottenVarPtr<SymbolicVariable> getSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        throw "bad!";
    }

    GottenVarPtr<SymbolicDouble> getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
        if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
        return GottenVarPtr<SymbolicDouble>(move(sa->operator[](index))); //WHY operator[]???!
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
    
    std::string getFullName() const override
    {
        return name + "[" + std::to_string(index) + "]";
    }

    std::string getUniqueID(SymbolTable& st) const override
    {
        return st.findIdentifier(name)->getUniqueID() + "[" + std::to_string(index) + "]";
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

    std::string getUniqueID(SymbolTable& st) const override
    {
        return st.findIdentifier(name)->getUniqueID() + "[" + index->getUniqueID(st) + "]";
    }

    std::unique_ptr<VarGetter> clone() override
    {
        return std::make_unique<GetSDByIndexVar>(name, index->clone());
    }
};

class SetArraySDByIndex: public VarSetter
{
public:
    const unsigned long index;

    SetArraySDByIndex(std::string n, const unsigned long& i): index(i) {setName(move(n));};

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

    std::string getFullName() const override
    {
        return name + "[" + std::to_string(index) + "]";
    }

    std::string getUniqueID(SymbolTable& st) const override
    {
        return st.findIdentifier(name)->getUniqueID() + "[" + std::to_string(index) + "]";
    }
};

class SetArrayByVar: public VarSetter
{
public:
    std::unique_ptr<VarGetter> index;

    SetArrayByVar(std::string arrN, std::unique_ptr<VarGetter> var):
            index(move(var)) {setName(move(arrN));}

    void setSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicDouble *sv) override
    {
        SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
        if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
        SymbolicDouble* ind = index->getSymbolicDouble(sef).get();
        sa->set(ind, sv);
        
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

    std::string getUniqueID(SymbolTable& st) const override
    {
        return st.findIdentifier(name)->getUniqueID() + "[" + index->getUniqueID(st) + "]";
    }
};

class SetSVByName: public VarSetter
{
public:
    SetSVByName(std::string n) {setName(move(n));};

    void setSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicDouble *sv) override
    {
        sv->setName(name);
        sef->symbolicVarSet->addVar(sv->clone());
    }

    std::string getFullName() const override
    {
        return name;
    }

    std::string getUniqueID(SymbolTable& st) const override
    {
        return st.findIdentifier(name)->getUniqueID();
    }
};
#endif //PROJECT_SYMBOLICVARWRAPPERS_H
