#ifndef PROJECT_SYMBOLICVARIABLE_H
#define PROJECT_SYMBOLICVARIABLE_H

template <typename T>
class SymbolicVariable
{
private:
    T* upperBound; //pointers to allow 'null' to signify no known bound
    T* lowerBound;
    MonotoneEnum monotinicity;
    T* minStep;

public:
    SymbolicVariable();
    ~SymbolicVariable();
    enum MonotoneEnum{INCREASING, DECREASING, CONST, FRESH, NONE, UNKNOWN};
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
    void addToConst(const T &);
    void setLowerBound(const T&);
    void setUpperBound(const T&);
    void setConstValue(const T&);
};


#endif
