#ifndef PROJECT_SYMBOLICVARIABLE_H
#define PROJECT_SYMBOLICVARIABLE_H

#include <memory>
#include <set>
#include <vector>

#include "../compile/Reporter.h"
#include "../compile/Token.h"
//SymbolicVariable.cpp

class VarWrapper;
namespace SymbolicExecution{class SymbolicExecutionFringe;};

class SymbolicVariable : public std::enable_shared_from_this<SymbolicVariable>
{
protected:
    bool defined = false;
    bool feasable = true;
    bool incrementable = false;
    bool userAffected = false;
    std::string varN;
    Reporter& reporter;
    VariableType type;
    //most of the time we will scan through all of these - hence vectors
    std::vector<SymbolicVariable*> lt;
    std::vector<SymbolicVariable*> le;
    std::vector<SymbolicVariable*> ge;
    std::vector<SymbolicVariable*> gt;
    std::vector<SymbolicVariable*> eq;
    std::vector<SymbolicVariable*> neq;

    void reportError(Reporter::AlertType type, std::string err);

public:
    enum MonotoneEnum{INCREASING, DECREASING, FRESH, NONE, UNKNOWN};
    enum MeetEnum {CANT, MAY, MUST};

    SymbolicVariable(std::string name, VariableType t, Reporter& r, bool defined, bool feasable, bool incrementable);
    SymbolicVariable(const SymbolicVariable& other) = delete;
    SymbolicVariable(const SymbolicVariable&& other) = delete;
    ~SymbolicVariable();
    Reporter& getReporter() const {return reporter;}

    const VariableType getType() const;
    const std::string& getName() const;
    void setName(const std::string& newName);
    bool isDefined() const;
    bool isIncrementable() const;
    bool wasUserAffected() const;
    void define();
    virtual MonotoneEnum getMonotonicity() const = 0;

    virtual bool guaranteedLT(SymbolicVariable* searchFor, const std::string& initName);
    virtual bool guaranteedLE(SymbolicVariable* searchFor, const std::string& initName);
    virtual bool guaranteedGT(SymbolicVariable* searchFor, const std::string& initName);
    virtual bool guaranteedGE(SymbolicVariable* searchFor, const std::string& initName);
    virtual bool guaranteedEQ(SymbolicVariable* searchFor, const std::string& initName);
    virtual bool guaranteedNEQ(SymbolicVariable* searchFor, const std::string& initName);

    virtual bool addLT(const VarWrapper* other, SymbolicExecution::SymbolicExecutionFringe* sef, bool constructed);
    virtual bool addLE(const VarWrapper* other, SymbolicExecution::SymbolicExecutionFringe* sef, bool constructed);
    virtual bool addGT(const VarWrapper* other, SymbolicExecution::SymbolicExecutionFringe* sef, bool constructed);
    virtual bool addGE(const VarWrapper* other, SymbolicExecution::SymbolicExecutionFringe* sef, bool constructed);
    virtual bool addEQ(const VarWrapper* other, SymbolicExecution::SymbolicExecutionFringe* sef, bool constructed);
    virtual bool addNEQ(const VarWrapper* other, SymbolicExecution::SymbolicExecutionFringe* sef, bool constructed);
    virtual void addNEQConst(const std::string& c) = 0;
    virtual void clearLess();
    virtual void clearGreater();
    virtual void clearEQ();
    virtual void clearAll();

    virtual SymbolicVariable::MeetEnum canMeet(Relations::Relop rel, const std::string& rhs) const = 0;
    virtual SymbolicVariable::MeetEnum canMeet(Relations::Relop rel, SymbolicVariable* rhs) const = 0;
    virtual const std::string getConstString() = 0;
    virtual void userInput() = 0;
    virtual void nondet() = 0;
    virtual void loopInit() {}
    virtual bool isBoundedBelow() const = 0;
    virtual bool isBoundedAbove() const = 0;
    virtual bool isDetermined() const = 0;
    virtual bool clipLowerBound(const std::string& lb, bool closed=true) = 0;
    virtual bool clipUpperBound(const std::string& ub, bool closed=true) = 0;
    virtual bool setUpperBound(const std::string& ub, bool closed=true) = 0;
    virtual bool setLowerBound(const std::string& ub, bool closed=true) = 0;
    virtual bool unionVar(SymbolicVariable* unionFrom) = 0;
    virtual void iterateTo(const std::string& to, bool closed=true) = 0;
    virtual void iterateTo(SymbolicVariable* to, bool closed=true) = 0;

    //these return true if there has been a change
    virtual bool unionLowerBound(const std::string& lb, bool closed=true) = 0;
    virtual bool unionUpperBound(const std::string& ub, bool closed=true) = 0;

    virtual std::string getUpperBound() const = 0;
    virtual std::string getLowerBound() const = 0;
    virtual void setConstValue(const std::string&) = 0;
    virtual bool meetsConstComparison(Relations::Relop r, const std::string& rhs) = 0;
    virtual bool isFeasable();

    virtual std::unique_ptr<SymbolicVariable> clone() = 0;
    virtual SymbolicVariable* cloneRaw() = 0;
};

template <typename T>
class SymbolicVariableTemplate : public SymbolicVariable
{
protected:
    T upperBound;
    T lowerBound;
    std::vector<T> neqConsts;

public:
    SymbolicVariableTemplate(std::string name, const T lower, const T upper,
                     Reporter& r, VariableType t, bool init, bool incrementable);

    bool isDisjointFrom(std::shared_ptr<SymbolicVariableTemplate<T>> other);
    bool meetsConstComparison(Relations::Relop r, const std::string& rhs) override;
    virtual SymbolicVariable::MeetEnum canMeet(Relations::Relop rel, SymbolicVariable* rhs) const override;

    virtual bool addEQ(const VarWrapper* other,
                       SymbolicExecution::SymbolicExecutionFringe* sef, bool constructed) override;
    void addNEQConst(const std::string& c) override;
    void clearEQ() override;

    //set/clip bounds returns true if var is feasable after
    virtual bool setTLowerBound(const T& lb, bool closed) = 0;
    virtual bool setTUpperBound(const T& ub, bool closed) = 0;
    virtual bool clipTLowerBound(const T& lb, bool closed) = 0;
    virtual bool clipTUpperBound(const T& up, bool closed) = 0;
    virtual bool unionTLowerBound(const T& lb, bool closed) = 0;
    virtual bool unionTUpperBound(const T& up, bool closed) = 0;
    virtual void unionTConstValue(const T& cv, bool closed) = 0;
    virtual void setTConstValue(const T& cv);
    const T& getTConstValue();
    const T& getTUpperBound() const;
    const T& getTLowerBound() const;
    std::string getUpperBound() const override;
    std::string getLowerBound() const override;
    const std::string getConstString() override;
    bool isFeasable() override;
    bool isDetermined() const override;
};

//SymbolicDouble.cpp
class SymbolicDouble : public SymbolicVariableTemplate<double>
{
private:
    long double minChange = 0; //most negative possible movement since initiation
    long double maxChange = 0; //most positive possible movement since initiation
    bool uniformlyChanging = true;
    void addConstToLower(const double d);
    void addConstToUpper(const double d);

public:
    SymbolicDouble(SymbolicDouble& o);
    SymbolicDouble(SymbolicVariable* o);
    SymbolicDouble(std::string name, Reporter& reporter);
    MeetEnum canMeet(Relations::Relop rel, const std::string& rhs) const override;
    SymbolicVariable::MeetEnum canMeet(Relations::Relop rel, SymbolicVariable* rhs) const override;
    std::unique_ptr<SymbolicVariable> clone() override;
    std::unique_ptr<SymbolicDouble> cloneSD();
    SymbolicVariable* cloneRaw() override;

    void removeUpperBound();
    void removeLowerBound();
    bool setTLowerBound(const double& d, bool closed = true) override;
    bool setTUpperBound(const double& d, bool closed = true) override;
    bool clipTLowerBound(const double& d, bool closed = true) override;
    bool clipTUpperBound(const double& d, bool closed = true) override;
    void setTConstValue(const double& d) override;
    bool unionTLowerBound(const double& lb, bool closed = true) override;
    bool unionTUpperBound(const double& up, bool closed = true) override;
    bool unionVar(SymbolicVariable* other) override;
    void setConstValue(const std::string&) override;
    bool clipUpperBound(const std::string& ub, bool closed = true) override;
    bool clipLowerBound(const std::string& lb, bool closed = true) override;
    bool unionUpperBound(const std::string& ub, bool closed = true) override;
    bool unionLowerBound(const std::string& lb, bool closed = true) override;
    void unionTConstValue(const double& cv, bool closed=true) override;
    bool setUpperBound(const std::string& ub, bool closed = true) override;
    bool setLowerBound(const std::string& lb, bool closed = true) override;
    virtual void iterateTo(double to, bool closed=true);
    virtual void iterateTo(const std::string& to, bool closed=true);
    virtual void iterateTo(SymbolicVariable* to, bool closed=true);

    void userInput() override;
    void nondet() override;

    void loopInit() override;
    bool isBoundedBelow() const override;
    bool isBoundedAbove() const override;
    MonotoneEnum getMonotonicity() const override;
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
    SymbolicString(SymbolicString& other);
    SymbolicString(SymbolicVariable* other);
    MeetEnum canMeet(Relations::Relop rel, const std::string& rhs) const override;
    MonotoneEnum getMonotonicity() const override {return NONE;}
    std::unique_ptr<SymbolicVariable> clone() override;
    SymbolicVariable* cloneRaw() override;

    bool setTLowerBound(const std::string&, bool closed = true) override;
    bool setTUpperBound(const std::string&, bool closed = true) override;
    bool clipTLowerBound(const std::string&, bool closed = true) override;
    bool clipTUpperBound(const std::string&, bool closed = true) override;
    bool unionTLowerBound(const std::string& lb, bool closed = true) override;
    bool unionTUpperBound(const std::string& up, bool closed = true) override;
    void setTConstValue(const std::string& s) override;
    void unionTConstValue(const std::string& s, bool closed=true) override;
    bool unionVar(SymbolicVariable* other) override;
    void setConstValue(const std::string&) override;
    bool clipUpperBound(const std::string& ub, bool closed = true) override;
    bool clipLowerBound(const std::string& lb, bool closed = true) override;
    bool unionUpperBound(const std::string& ub, bool closed = true) override;
    bool unionLowerBound(const std::string& lb, bool closed = true) override;
    bool setUpperBound(const std::string& ub, bool closed = true) override;
    bool setLowerBound(const std::string& lb, bool closed = true) override;
    void iterateTo(const std::string& to, bool closed) {throw "not changing";};
    void iterateTo(SymbolicVariable* sv, bool closed) {throw "not changing";}

    void userInput() override;
    void nondet() override;

    bool isBoundedBelow() const override;
    bool isBoundedAbove() const override;
};


#endif
