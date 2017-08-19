#ifndef PROJECT_SYMBOLICVARIABLE_H
#define PROJECT_SYMBOLICVARIABLE_H

#include <memory>

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
    bool isInitialised;
    bool feasable;
    std::string varN;
    Reporter& reporter;
    VariableType type;

    void reportError(Reporter::AlertType type, std::string err);

public:
    SymbolicVariable(std::string name, VariableType t, Reporter& r, bool initialised = false);
    const VariableType getType() const;
    virtual bool isDetermined();
    const std::string getName() const;
    virtual void userInput() = 0;
    virtual bool isFeasable() = 0;
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
    virtual void userInput() = 0;
    virtual bool isBoundedBelow() const = 0;
    virtual bool isBoundedAbove() const = 0;
    bool isFeasable();
    bool isDetermined();
    const T getUpperBound() const;
    const T getLowerBound() const;
    void setLowerBound(const T);
    void setUpperBound(const T);
    void setConstValue(const T);
    const T getConstValue();
};

/*
class SymbolicVariablePointer
{
private:
    VariableType type;
    union
    {
        std::shared_ptr<SymbolicDouble> doublePtr;
        std::shared_ptr<SymbolicString> stringPtr;
    };
public:
    SymbolicVariablePointer();
    SymbolicVariablePointer(std::shared_ptr<SymbolicDouble> dptr);
    SymbolicVariablePointer(std::shared_ptr<SymbolicString> sptr);
    SymbolicVariablePointer(SymbolicVariablePointer& other);
    SymbolicVariablePointer(std::shared_ptr<SymbolicVariablePointer> other);
    ~SymbolicVariablePointer();

    VariableType getType();
    std::shared_ptr<SymbolicDouble> getDoublePointer();
    std::shared_ptr<SymbolicString> getStringPointer();
};*/

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
    SymbolicDouble(SymbolicDouble& other);
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
    //bool canMerge(const SymbolicDouble& other) const;
};

//SymbolicString.cpp
class SymbolicString : public SymbolicVariableTemplate<std::string>
{
private:
    bool boundedLower;
    bool boundedUpper;

public:
    SymbolicString(std::string name, Reporter& reporter);
    SymbolicString(std::shared_ptr<SymbolicString> other);
    SymbolicString(SymbolicString& other);

    void userInput();
    bool isBoundedBelow() const;
    bool isBoundedAbove() const;
};


#endif
