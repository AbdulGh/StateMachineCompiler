#ifndef PROJECT_SYMBOLICDOUBLE_H
#define PROJECT_SYMBOLICDOUBLE_H

#include <memory>
#include <set>
#include <vector>

#include "../compile/Reporter.h"
#include "../compile/Token.h"
#include "../Command.h"
//SymbolicDouble.cpp

class VarWrapper;
namespace SymbolicExecution{class SymbolicExecutionFringe;};

class SymbolicDouble : public std::enable_shared_from_this<SymbolicDouble>
{
private:
    bool defined = false;
    bool feasable = true;
    bool userAffected = false;
    std::string varN;
    Reporter& reporter;

    std::set<SymbolicDouble*> lt;
    std::set<SymbolicDouble*> le;
    std::set<SymbolicDouble*> ge;
    std::set<SymbolicDouble*> gt;
    std::set<SymbolicDouble*> eq;
    std::set<SymbolicDouble*> neq;

    void reportError(Reporter::AlertType type, std::string err);

    double upperBound;
    double lowerBound;
    double repeatUpper;
    double repeatLower;
    std::set<double> neqConsts;
    long double minChange = 0; //most negative possible movement since initiation
    long double maxChange = 0; //most positive possible movement since initiation
    bool uniformlyChanging = true;
    void addConstToLower(const double d);
    void addConstToUpper(const double d);
    
public:
    enum MonotoneEnum{INCREASING, DECREASING, FRESH, NONE, UNKNOWN};
    enum MeetEnum {CANT, MAY, MUST};

    SymbolicDouble(std::string name, Reporter& reporter);
    SymbolicDouble(const SymbolicDouble& o);
    SymbolicDouble(SymbolicDouble* o);
    SymbolicDouble(const SymbolicDouble&& other) = delete;
    SymbolicDouble& operator=(const SymbolicDouble& o);

    ~SymbolicDouble();
    Reporter& getReporter() const {return reporter;}

    const std::string& getName() const;
    void setName(const std::string& newName);
    bool isDefined() const;
    virtual bool getRelativeVelocity(SymbolicDouble* o,long double& slowest, long double& fastest) const;
    bool wasUserAffected() const;
    void define();
    
    virtual bool guaranteedLT(SymbolicDouble* searchFor, SymbolicDouble* searchInit, std::set<SymbolicDouble*>& seen);
    virtual bool guaranteedLE(SymbolicDouble* searchFor, SymbolicDouble* searchInit, std::set<SymbolicDouble*>& seen);
    virtual bool guaranteedGT(SymbolicDouble* searchFor, SymbolicDouble* searchInit, std::set<SymbolicDouble*>& seen);
    virtual bool guaranteedGE(SymbolicDouble* searchFor, SymbolicDouble* searchInit, std::set<SymbolicDouble*>& seen);
    virtual bool guaranteedEQ(SymbolicDouble* searchFor, SymbolicDouble* searchInit, std::set<SymbolicDouble*>& seen);
    virtual bool guaranteedNEQ(SymbolicDouble* searchFor, SymbolicDouble* searchInit, std::set<SymbolicDouble*>& seen);

    virtual bool addLT(const VarWrapper* other, SymbolicExecution::SymbolicExecutionFringe* sef, bool constructed);
    virtual bool addLE(const VarWrapper* other, SymbolicExecution::SymbolicExecutionFringe* sef, bool constructed);
    virtual bool addGT(const VarWrapper* other, SymbolicExecution::SymbolicExecutionFringe* sef, bool constructed);
    virtual bool addGE(const VarWrapper* other, SymbolicExecution::SymbolicExecutionFringe* sef, bool constructed);
    virtual bool addEQ(const VarWrapper* other, SymbolicExecution::SymbolicExecutionFringe* sef, bool constructed);
    virtual bool addNEQ(const VarWrapper* other, SymbolicExecution::SymbolicExecutionFringe* sef, bool constructed);
    virtual void clearLess();
    virtual void clearGreater();
    virtual void clearEQ();
    virtual void clearAll();

    //these return true if there has been a change
    virtual bool isFeasable();

    MeetEnum canMeet(Relations::Relop rel, double rhs) const;
    SymbolicDouble::MeetEnum canMeet(Relations::Relop rel, SymbolicDouble* rhs) const;
    std::unique_ptr<SymbolicDouble> clone();
    SymbolicDouble* cloneRaw();

    void maxUpperBound(); void removeUpperBound();
    void minLowerBound(); void removeLowerBound();
    bool setLowerBound(double d, short direction = 0);
    bool setUpperBound(double d, short direction = 0);
    bool clipLowerBound(double d, short direction = 0);
    bool clipUpperBound(double d, short direction = 0);
    void setConstValue(double d);
    bool unionLowerBound(double lb, short direction = 0);
    bool unionUpperBound(double up, short direction = 0);
    bool unionVar(const SymbolicDouble* other);
    bool unionConstValue(double cv, short direction);
    void iterateTo(double to, short direction=true);
    void iterateTo(SymbolicDouble* to, short direction=true);

    double getLowerBound() const;
    double getUpperBound() const;
    double getRepeatLowerBound() const;
    double getRepeatUpperBound() const;

    bool operator==(const SymbolicDouble& o)
    {
        return upperBound == o.upperBound && lowerBound == o.lowerBound
               && repeatUpper == o.repeatUpper && repeatLower == o.repeatLower;
    }

    double getConstValue() const;
    bool isDetermined() const;

    void setRepeatBoundsFromComparison(Relations::Relop r, double rhs);
    void setRepeatBoundsFromComparison(Relations::Relop r, SymbolicDouble* rhs);

    void setRepeatLowerBound(double lb, short direction = 0);
    void setRepeatUpperBound(double up, short direction = 0);
    void removeRepeatLowerBound();
    void removeRepeatUpperBound();
    void resetRepeatBounds();

    void userInput();
    void nondet();

    void loopInit();
    bool isBoundedBelow() const;
    bool isBoundedAbove() const;
    MonotoneEnum getMonotonicity() const;
    void addConst(double);
    void multConst(double);
    void divConst(double);
    void modConst(double);
    void addSymbolicDouble(SymbolicDouble& other, bool increment = false);
    void minusSymbolicDouble(SymbolicDouble& other, bool increment = false);
    void multSymbolicDouble(SymbolicDouble& other);
    void divSymbolicDouble(SymbolicDouble& other);
    void modSymbolicDouble(SymbolicDouble& other);
};

#endif
