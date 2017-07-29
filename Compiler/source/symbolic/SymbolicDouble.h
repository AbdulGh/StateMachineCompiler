#ifndef PROJECT_SYMBOLICVARIABLE_H
#define PROJECT_SYMBOLICVARIABLE_H

#include "../compile/Reporter.h"

enum MonotoneEnum{INCREASING, DECREASING, FRESH, NONE, UNKNOWN};

//todo redo multiplication
//todo 'start recording' - make monotonicity fresh
class SymbolicDouble
{
private:
    double upperBound;
    double lowerBound;
    MonotoneEnum monotonicity;
    double minStep;
    bool isConst;
    bool isInitialised;
    bool feasable;
    std::string varN;
    Reporter& reporter;

    void addConstToLower(const double d);
    void addConstToUpper(const double d);
    void reportError(Reporter::AlertType type, std::string err);
    //void multLower(const double d);
    //void multUpper(const double d);

public:
    SymbolicDouble(std::string name, Reporter& reporter);
    SymbolicDouble(SymbolicDouble& other);
    bool isBoundedBelow() const;
    bool isBoundedAbove() const;
    bool isFeasable();
    void userInput();
    const double getUpperBound() const;
    const double getLowerBound() const;
    const MonotoneEnum getMonotinicity() const;
    const double getMininumStep() const;
    const double getConstValue() const;
    bool isDetermined() const;
    void addConst(double);
    void multConst(double);
    void divConst(double); //todo
    void modConst(double);
    void addSymbolicDouble(SymbolicDouble& other);
    void multSymbolicDouble(SymbolicDouble& other);
    void divSymbolicDouble(SymbolicDouble& other); //todo
    void modSymbolicDouble(SymbolicDouble& other);
    void setLowerBound(const double);
    void setUpperBound(const double);
    void setConstValue(const double);
    //bool canMerge(const SymbolicDouble& other) const;
};


#endif
