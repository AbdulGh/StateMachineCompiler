#ifndef PROJECT_SYMBOLICVARIABLE_H
#define PROJECT_SYMBOLICVARIABLE_H

#include <memory>
#include <set>

#include "../compile/Reporter.h"
#include "../compile/Token.h"
//SymbolicVariable.cpp

class SymbolicVariable
{
protected:
    bool isConst;
    bool defined;
    bool feasable = true;
    std::string varN;
    Reporter& reporter;
    VariableType type;
    //std::set<std::shared_ptr<SymbolicVariable>> lessThan; todo maintain lists of relations to other variables

    void reportError(Reporter::AlertType type, std::string err);

public:
    enum MonotoneEnum{INCREASING, DECREASING, FRESH, NONE}; //represents what we know - 'NONE' might still be increasing etc
    enum MeetEnum {CANT, MAY, MUST};
    SymbolicVariable(std::string name, VariableType t, Reporter& r, bool initialised = false);
    const VariableType getType() const;
    virtual bool isDetermined();
    const std::string getName() const;
    void setName(const std::string newName);
    bool isDefined() const;
    void define();
    virtual MonotoneEnum getMonotonicity() const = 0;

    virtual SymbolicVariable::MeetEnum canMeet(Relations::Relop rel, const std::string& rhs) const = 0;
    virtual const std::string getConstString() = 0;
    virtual void userInput() = 0;
    virtual bool isBoundedBelow() const = 0;
    virtual bool isBoundedAbove() const = 0;
    virtual bool clipLowerBound(const std::string& lb, bool closed=true) = 0;
    virtual bool clipUpperBound(const std::string& ub, bool closed=true) = 0;
    virtual bool unionLowerBound(const std::string& lb, bool closed=true) = 0;
    virtual bool unionUpperBound(const std::string& ub, bool closed=true) = 0;
    virtual bool setUpperBound(const std::string& ub, bool closed=true) = 0;
    virtual bool setLowerBound(const std::string& ub, bool closed=true) = 0;
    virtual void setConstValue(const std::string&) = 0;
    virtual bool meetsConstComparison(Relations::Relop r, const std::string& rhs) = 0;
    virtual bool isFeasable() const;

    virtual std::shared_ptr<SymbolicVariable> clone() = 0;
};

template <typename T>
class SymbolicVariableTemplate : public SymbolicVariable
{
protected:
    T upperBound;
    T lowerBound;

public:
    SymbolicVariableTemplate(std::string name, const T lower, const T upper,
                     Reporter& r, VariableType t, bool init=false);

    bool isDisjointFrom(std::shared_ptr<SymbolicVariableTemplate<T>> other);
    bool meetsConstComparison(Relations::Relop r, const std::string& rhs) override;
    //set/clip bounds returns true if var is feasable after
    virtual bool setTLowerBound(const T& lb, bool closed) = 0;
    virtual bool setTUpperBound(const T& ub, bool closed) = 0;
    virtual bool clipTLowerBound(const T& lb, bool closed) = 0;
    virtual bool clipTUpperBound(const T& up, bool closed) = 0;
    virtual bool unionTLowerBound(const T& lb, bool closed) = 0;
    virtual bool unionTUpperBound(const T& up, bool closed) = 0;
    const T& getUpperBound() const;
    virtual void setTConstValue(const T& cv);
    const T& getLowerBound() const;
    const T& getConstValue();
    const std::string getConstString() override;
    bool isFeasable();
    bool isDetermined() override;
};
//todo check comparison strictness wrt closed bool when setting bounds
//SymbolicDouble.cpp
class SymbolicDouble : public SymbolicVariableTemplate<double>
{
private:
    double minStep;
    MonotoneEnum monotonicity;
    void addConstToLower(const double d);
    void addConstToUpper(const double d);

public:
    SymbolicDouble(std::string name, Reporter& reporter);
    SymbolicDouble(std::shared_ptr<SymbolicDouble> other);
    SymbolicDouble(std::shared_ptr<SymbolicVariable> other);
    SymbolicDouble(SymbolicDouble& other);
    MeetEnum canMeet(Relations::Relop rel, const std::string& rhs) const override;
    std::shared_ptr<SymbolicVariable> clone() override;
    
    bool setTLowerBound(const double& d, bool closed = true) override;
    bool setTUpperBound(const double& d, bool closed = true) override;
    bool clipTLowerBound(const double& d, bool closed = true) override;
    bool clipTUpperBound(const double& d, bool closed = true) override;
    bool unionTLowerBound(const double& lb, bool closed = true) override;
    bool unionTUpperBound(const double& up, bool closed = true) override;
    void setConstValue(const std::string&) override;
    bool clipUpperBound(const std::string& ub, bool closed = true) override;
    bool clipLowerBound(const std::string& lb, bool closed = true) override;
    bool unionUpperBound(const std::string& ub, bool closed = true) override;
    bool unionLowerBound(const std::string& lb, bool closed = true) override;
    bool setUpperBound(const std::string& ub, bool closed = true) override;
    bool setLowerBound(const std::string& lb, bool closed = true) override;

    void userInput() override ;
    bool isBoundedBelow() const override ;
    bool isBoundedAbove() const override ;
    const double getMininumStep() const {return minStep;};
    MonotoneEnum getMonotonicity() const override {return monotonicity;};
    void addConst(double);
    void multConst(double);
    void divConst(double);
    void modConst(double);
    void addSymbolicDouble(SymbolicDouble& other);
    void multSymbolicDouble(SymbolicDouble& other);
    void divSymbolicDouble(SymbolicDouble& other);
    void modSymbolicDouble(SymbolicDouble& other);
};

//SymbolicString.cpp
class SymbolicString : public SymbolicVariableTemplate<std::string>
{
private:
    bool boundedLower;
    bool boundedUpper;

    static std::string incrementString(std::string s);
    static std::string decrementString(std::string s);

public:
    SymbolicString(std::string name, Reporter& reporter);
    SymbolicString(std::shared_ptr<SymbolicString> other);
    SymbolicString(std::shared_ptr<SymbolicVariable> other);
    SymbolicString(SymbolicString& other);
    MeetEnum canMeet(Relations::Relop rel, const std::string& rhs) const override;
    MonotoneEnum getMonotonicity() const override {return NONE;}
    std::shared_ptr<SymbolicVariable> clone() override;

    bool setTLowerBound(const std::string&, bool closed = true) override;
    bool setTUpperBound(const std::string&, bool closed = true) override;
    bool clipTLowerBound(const std::string&, bool closed = true) override;
    bool clipTUpperBound(const std::string&, bool closed = true) override;
    bool unionTLowerBound(const std::string& lb, bool closed = true) override;
    bool unionTUpperBound(const std::string& up, bool closed = true) override;
    void setTConstValue(const std::string& s) override;
    void setConstValue(const std::string&) override;
    bool clipUpperBound(const std::string& ub, bool closed = true) override;
    bool clipLowerBound(const std::string& lb, bool closed = true) override;
    bool unionUpperBound(const std::string& ub, bool closed = true) override;
    bool unionLowerBound(const std::string& lb, bool closed = true) override;
    bool setUpperBound(const std::string& ub, bool closed = true) override;
    bool setLowerBound(const std::string& lb, bool closed = true) override;

    void userInput() override;
    bool isBoundedBelow() const override;
    bool isBoundedAbove() const override;
};


#endif
