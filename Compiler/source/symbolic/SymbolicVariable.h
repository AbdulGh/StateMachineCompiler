#ifndef PROJECT_SYMBOLICVARIABLE_H
#define PROJECT_SYMBOLICVARIABLE_H

enum MonotoneEnum{INCREASING, DECREASING, CONSTCONST, CONSTINCREASING, CONSTDECREASING, FRESH, NONE, UNKNOWN};

template <typename T>
class SymbolicVariable
{
private:
    T* upperBound; //pointers to allow 'null' to signify no known bound
    T* lowerBound;
    MonotoneEnum monotonicity;
    T* minStep;

public:
    SymbolicVariable();
    SymbolicVariable(SymbolicVariable<T>& other);
    ~SymbolicVariable();
    bool isBoundedAbove() const;
    bool isBoundedBelow() const;
    bool isFeasable() const;
    void reset();
    const T getUpperBound() const;
    const T getLowerBound() const;
    const MonotoneEnum getMonotinicity() const;
    const T getMininumStep() const;
    const T getConstValue() const;
    bool isDetermined() const;
    void addConst(T);
    void multConst(T);
    void modConst(T);
    void setLowerBound(const T&);
    void setUpperBound(const T&);
    void setConstValue(const T&);
    //bool canMerge(const SymbolicVariable& other) const;
};


#endif
