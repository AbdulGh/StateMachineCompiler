//
// Created by abdul on 03/08/17.
//

#ifndef PROJECT_SYMBOLICSTACK_H
#define PROJECT_SYMBOLICSTACK_H

#include <vector>

#include "SymbolicVariables.h"

enum class SymbolicStackMemberType {STATE, VAR};

class StackMember
{
protected:
    SymbolicStackMemberType type;
    void setType(SymbolicStackMemberType t) {type = t;}

public:
    virtual std::unique_ptr<StackMember> clone() = 0;
    virtual std::string diagString() = 0;
    virtual void mergeSM(std::unique_ptr<StackMember>& other) = 0;
    virtual const std::string& getName() {throw "not implemented in this type";}

    SymbolicStackMemberType getType() const {return type;}
};

class SymVarStackMember : public StackMember
{
public:
    std::unique_ptr<SymbolicVariable> varptr;

    SymVarStackMember(std::unique_ptr<SymbolicVariable> vp):
            varptr(move(vp))
    {
        setType(SymbolicStackMemberType::VAR);
    }

    explicit SymVarStackMember(SymbolicVariable* toPush)
    {
        varptr = toPush->clone();
        setType(SymbolicStackMemberType::VAR);
    }

    explicit SymVarStackMember(double toPush)
    {
        //execution defined vars shouldnt throw errors (so nullptr is okay)
        std::unique_ptr<SymbolicDouble> sd = std::make_unique<SymbolicDouble>("constDouble", nullptr);
        sd->setTConstValue(toPush);
        varptr = std::move(sd);
    }

    explicit SymVarStackMember(const std::string& toPush)
    {
        varptr = std::make_unique<SymbolicString>("constString", nullptr);
        varptr->setConstValue(toPush);
    }

    std::unique_ptr<StackMember> clone() override
    {
        return std::make_unique<SymVarStackMember>(varptr.get());
    }

    void mergeSM(std::unique_ptr<StackMember>& other) override
    {
        if (other->getType() != SymbolicStackMemberType::VAR) throw "merged incompatable types";
        SymVarStackMember* o = static_cast<SymVarStackMember*>(other.get());
        varptr->unionUpperBound(o->varptr->getUpperBound());
        varptr->unionLowerBound(o->varptr->getLowerBound());
        if (!varptr->isDetermined()) setType(SymbolicStackMemberType::VAR);
    }

    void mergeVar(SymbolicVariable& ovarptr)
    {
        varptr->unionUpperBound(ovarptr.getUpperBound());
        varptr->unionLowerBound(ovarptr.getLowerBound());
        if (!varptr->isDetermined()) setType(SymbolicStackMemberType::VAR);
    }
    
    std::string diagString() override {return "var " + varptr->getName();}
    const std::string& getName() override {return varptr->getName();}
};

class StateStackMember : public StackMember
{
private:
    std::string stateName;
public:
    explicit StateStackMember(std::string stateName): stateName(std::move(stateName))
    {setType(SymbolicStackMemberType::STATE);}

    std::unique_ptr<StackMember> clone() override
    {
        return std::make_unique<StateStackMember>(stateName);
    }

    const std::string& getName() override {return stateName;}

    std::string diagString() override {return "state " + stateName;}

    void mergeSM(std::unique_ptr<StackMember>& other) override
    {
        if (other->getType() != SymbolicStackMemberType::STATE) throw "wrong types";
        if (other->getName() != stateName) throw "todo next";
    }
};

class SymbolicStack
{
private:
    std::shared_ptr<SymbolicStack> parent;
    std::vector<std::unique_ptr<StackMember>> currentStack;
    Reporter& reporter;
    void copyParent();
    std::unique_ptr<StackMember> popMember();
public:
    SymbolicStack(Reporter& r);
    SymbolicStack(std::shared_ptr<SymbolicStack> parent);
    SymbolicStack(const SymbolicStack&) = delete;
    //void push(std::unique_ptr<SymbolicVariable> pushedVar);
    void pushVar(SymbolicVariable *pushedVar);
    void pushState(const std::string& pushedState);
    void pushString(std::string toPush);
    void pushDouble(double toPush);
    std::string popState();
    std::string popString();
    double popDouble();
    void copyStack(SymbolicStack* other);
    void unionStack(SymbolicStack* other);
    std::unique_ptr<SymbolicVariable> popVar();
    SymbolicVariable* peekTopVar();
    const std::string& peekTopName();
    void pop();
    bool isEmpty();
    SymbolicStackMemberType getTopType();
    std::string printStack(); //diagnostic purposes
};


#endif //PROJECT_SYMBOLICSTACK_H
