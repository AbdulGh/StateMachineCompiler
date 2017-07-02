#ifndef PROJECT_SYMBOLICVARIABLE_H
#define PROJECT_SYMBOLICVARIABLE_H

template <typename T>
class SymbolicVariable //todo monotone/bound
{
private:
    T lowerBound;
    T upperBound;
public:
    T getLowerBound() const;
    T getUpperBound() const;

    bool isFeasable();
    void clipLower(T); //sets the lower bound to be at least the argument
    void clipUpper(T);

};


#endif
