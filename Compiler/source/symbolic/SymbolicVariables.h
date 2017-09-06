#ifndef PROJECT_SYMBOLICVARIABLE_H
#define PROJECT_SYMBOLICVARIABLE_H

#include <memory>
#include <set>

#include "../compile/Reporter.h"
#include "../compile/Token.h"

enum MonotoneEnum{INCREASING, DECREASING, FRESH, NONE, UNKNOWN};

//todo redo multiplication
//todo 'start recording' - make monotonicity fresh

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
    enum MeetEnum {CANT, MAY, MUST};
    SymbolicVariable(std::string name, VariableType t, Reporter& r, bool initialised = false);
    const VariableType getType() const;
    virtual bool isDetermined();
    const std::string getName() const;
    void setName(const std::string newName);
    bool isDefined() const;
    void define();

    virtual SymbolicVariable::MeetEnum canMeet(Relations::Relop rel, const std::string& rhs) const = 0;
    virtual void setConstStringValue(const std::string &) = 0;
    virtual const std::string getConstString() = 0;
    virtual void userInput() = 0;
    virtual bool isBoundedBelow() const = 0;
    virtual bool isBoundedAbove() const = 0;
    virtual bool meetsConstComparison(Relations::Relop r, const std::string& rhs) = 0;
    virtual bool isFeasable() const;
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
    virtual bool setLowerBound(const T& lb, bool closed=true) = 0;
    virtual bool setUpperBound(const T& ub, bool closed=true) = 0;
    virtual bool clipLowerBound(const T& lb, bool closed=true) = 0;
    virtual bool clipUpperBound(const T& up, bool closed=true) = 0;
    const T& getUpperBound() const;
    virtual void setConstValue(const T& cv);
    const T& getLowerBound() const;
    const T& getConstValue();
    const std::string getConstString();
    bool isFeasable();
    bool isDetermined();
};

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
    
    bool setLowerBound(const double& d, bool closed=true);
    bool setUpperBound(const double& d, bool closed=true);
    bool clipLowerBound(const double& d, bool closed=true);
    bool clipUpperBound(const double& d, bool closed=true);
    void setConstStringValue(const std::string&) override;

    void userInput();
    bool isBoundedBelow() const;
    bool isBoundedAbove() const;
    const double getMininumStep() const;
    const MonotoneEnum getMonotinicity() const;
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

    bool setLowerBound(const std::string&, bool closed=true) override;
    bool setUpperBound(const std::string&, bool closed=true) override;
    bool clipLowerBound(const std::string&, bool closed=true) override;
    bool clipUpperBound(const std::string&, bool closed=true) override;
    void setConstValue(const std::string& s) override;
    void setConstStringValue(const std::string& s) override;

    void userInput();
    bool isBoundedBelow() const;
    bool isBoundedAbove() const;
};


#endif
