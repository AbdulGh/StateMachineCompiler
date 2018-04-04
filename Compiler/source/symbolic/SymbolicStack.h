//
// Created by abdul on 03/08/17.
//

#ifndef PROJECT_SYMBOLICSTACK_H
#define PROJECT_SYMBOLICSTACK_H

#include <vector>

#include "VarWrappers.h"
#include "SymbolicDouble.h"

enum class SymbolicStackMemberType {STATE, VAR};

class StackMember
{
protected:
    SymbolicStackMemberType type;
    void setType(SymbolicStackMemberType t) {type = t;}

public:
    virtual std::unique_ptr<StackMember> clone() = 0;
    virtual std::string diagString() = 0;
    virtual void setLoopInit() {};
    virtual bool mergeSM(std::unique_ptr<StackMember>& other) = 0;
    virtual const std::string& getName() {throw std::runtime_error("not implemented in this type");}

    SymbolicStackMemberType getType() const {return type;}
};

class SymVarStackMember : public StackMember
{
public:
    std::unique_ptr<SymbolicDouble> varptr;

    explicit SymVarStackMember(std::unique_ptr<SymbolicDouble> vp):
            varptr(move(vp))
    {
        setType(SymbolicStackMemberType::VAR);
    }

    explicit SymVarStackMember(SymbolicDouble* toPush)
    {
        varptr = move(toPush->clone());
        setType(SymbolicStackMemberType::VAR);
    }

    explicit SymVarStackMember(GottenVarPtr<SymbolicDouble> toPush)
    {
        setType(SymbolicStackMemberType::VAR);
        if (toPush.constructed()) varptr = toPush.get()->clone();
        else varptr = toPush->clone();
    }

    explicit SymVarStackMember(double toPush, Reporter& r)
    {
        std::unique_ptr<SymbolicDouble> sd = std::make_unique<SymbolicDouble>("constDouble", r);
        sd->setConstValue(toPush);
        varptr = std::move(sd);
        setType(SymbolicStackMemberType::VAR);
    }

    std::unique_ptr<StackMember> clone() override
    {
        return std::make_unique<SymVarStackMember>(varptr.get());
    }

    std::unique_ptr<SymVarStackMember> cloneVarMember()
    {
        return std::make_unique<SymVarStackMember>(varptr.get());
    }

    void setLoopInit() override {varptr->loopInit();}

    bool mergeSM(std::unique_ptr<StackMember>& other) override
    {
        if (other->getType() != SymbolicStackMemberType::VAR) throw std::runtime_error("merged incompatable types");
        bool change = false;
        SymVarStackMember* o = static_cast<SymVarStackMember*>(other.get());

        if (varptr->unionUpperBound(o->varptr->getUpperBound())) change = true;
        if (varptr->unionLowerBound(o->varptr->getLowerBound())) change = true;
        if (!varptr->isDetermined()) setType(SymbolicStackMemberType::VAR);
        return change;
    }
    
    std::string diagString() override
    {
        return "var [" + std::to_string(varptr->getLowerBound()) + ", "
                       + std::to_string(varptr->getUpperBound()) + "] " + varptr->getName();
    }
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

    bool mergeSM(std::unique_ptr<StackMember>& other) override
    {
        throw std::runtime_error("cant merge state names");
    }
};

class StateListStackMember : public StackMember
{
private:
    std::set<std::string> stateNames;
public:
    explicit StateListStackMember(std::string stateName)
    {
        stateNames.insert(move(stateName));
        setType(SymbolicStackMemberType::STATE);
    }

    explicit StateListStackMember(std::set<std::string> stateNameList)
    {
        stateNames = move(stateNameList);
        setType(SymbolicStackMemberType::STATE);
    }

    std::unique_ptr<StackMember> clone() override
    {
        return std::make_unique<StateListStackMember>(stateNames);
    }

    bool mergeSM(std::unique_ptr<StackMember>& other) override
    {
        bool change = false;
        StateStackMember* o = static_cast<StateStackMember*>(other.get());
        return stateNames.insert(other->getName()).second;
    }

    std::string diagString() override {return "state list";}
};

class SymbolicStack
{
private:
    std::shared_ptr<SymbolicStack> parent;
    Reporter& reporter;
    bool loopInit = false;
    void copyParent();
    std::unique_ptr<StackMember> popMember();
public:
    std::vector<std::unique_ptr<StackMember>> currentStack;
    SymbolicStack(Reporter& r);
    SymbolicStack(std::shared_ptr<SymbolicStack> parent);
    SymbolicStack(const SymbolicStack&) = delete;
    void setLoopInit();
    //void push(std::unique_ptr<SymbolicDouble> pushedVar);
    void pushVar(SymbolicDouble* pushedVar);
    void pushVar(GottenVarPtr<SymbolicDouble> pushedVar);
    void pushState(const std::string& pushedState);
    void pushDouble(double toPush);
    std::string popState();
    std::shared_ptr<SymbolicStack>& getParent() {return parent;}

    const std::string getReturnState();
    void copyStack(SymbolicStack* other);
    bool assimilateChanges(SymbolicStack* other); //only until last state
    std::unique_ptr<SymbolicDouble> popVar();
    SymbolicDouble* peekTopVar();
    const std::string& peekTopName();
    void pop();
    bool isEmpty();
    SymbolicStackMemberType peekTopType();
    std::string printStack(); //diagnostic purposes
};


#endif //PROJECT_SYMBOLICSTACK_H
