//
// Created by abdul on 03/08/17.
//

#ifndef PROJECT_SYMBOLICSTACK_H
#define PROJECT_SYMBOLICSTACK_H

#include <vector>

#include "SymbolicVariables.h"

enum class SymbolicStackMemberType {STATE, VAR, STRING, DOUBLE};

class StackMember
{
protected:
    SymbolicStackMemberType type;
    explicit StackMember(SymbolicStackMemberType t) : type(t) {}

public:
    virtual std::unique_ptr<StackMember> clone() = 0;
    virtual std::string diagString() = 0;
    virtual const std::string& getName() {throw "not implemented in this type";}

    SymbolicStackMemberType getType() {return type;}
};

class SymVarStackMember : public StackMember
{
public:
    std::unique_ptr<SymbolicVariable> varptr;

    explicit SymVarStackMember(SymbolicVariable* toPush): StackMember(SymbolicStackMemberType::VAR)
    {
        if (toPush->getType() == DOUBLE) varptr = std::make_unique<SymbolicDouble>(toPush);
        else if (toPush->getType() == STRING) varptr = std::make_unique<SymbolicString>(toPush);
        else throw "bad dtype";
    }

    std::unique_ptr<StackMember> clone() override
    {
        return std::make_unique<SymVarStackMember>(varptr.get());
    }

    
    std::string diagString() override {return "var " + varptr->getName();}
    const std::string& getName() override {return varptr->getName();}
};

class DoubleStackMember : public StackMember
{
public:
    double d;
    explicit DoubleStackMember(double toPush): StackMember(SymbolicStackMemberType::DOUBLE), d(toPush)
    {}

    std::unique_ptr<StackMember> clone() override
    {
        return std::make_unique<DoubleStackMember>(d);
    }

    std::string diagString() override {return "double " + std::to_string(d);}
};

class StringStackMember : public StackMember
{
public:
    std::string s;
    explicit StringStackMember(std::string toPush): StackMember(SymbolicStackMemberType::STRING), s(std::move(toPush))
    {}

    std::unique_ptr<StackMember> clone() override
    {
        return std::make_unique<StringStackMember>(s); //copies
    }

    std::string diagString() override {return "string " + s;}
};

class StateStackMember : public StackMember //too much like the StringStackMember!
{
private:
    std::string stateName;
public:
    explicit StateStackMember(std::string stateName): StackMember(SymbolicStackMemberType::STATE),
                                                      stateName(std::move(stateName))
    {}

    std::unique_ptr<StackMember> clone() override
    {
        return std::make_unique<StateStackMember>(stateName);
    }

    const std::string& getName() override {return stateName;}

    std::string diagString() override {return "state " + stateName;}
    };

class SymbolicStack
{
private:
    std::shared_ptr<SymbolicStack> parent;
    std::vector<std::unique_ptr<StackMember>> currentStack;

    void copyParent();
    std::unique_ptr<StackMember> popMember();
public:
    SymbolicStack(std::shared_ptr<SymbolicStack> parent = nullptr);
    SymbolicStack(const SymbolicStack&) = delete;
    //void push(std::unique_ptr<SymbolicVariable> pushedVar);
    void pushVar(SymbolicVariable *pushedVar);
    void pushState(const std::string& pushedState);
    void pushString(std::string toPush);
    void pushDouble(double toPush);
    std::string popState();
    std::string popString();
    double popDouble();
    std::unique_ptr<SymbolicVariable> popVar();
    SymbolicVariable* peekTopVar();
    const std::string& peekTopName();
    void pop();
    bool isEmpty();
    SymbolicStackMemberType getTopType();
    std::string printStack(); //diagnostic purposes
};


#endif //PROJECT_SYMBOLICSTACK_H
