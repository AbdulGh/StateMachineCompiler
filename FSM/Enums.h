#ifndef ENUMS_H
#define ENUMS_H

enum ComparisonOp{GT, GE, LT, LE, EQ, NEQ};

template <typename T>
bool evaluateComparisonOp(T LHS, ComparisonOp op, T RHS)
{
    switch(op)
    {
        case GT:
            return LHS > RHS;
        case GE:
            return LHS >= RHS;
        case LT:
            return LHS < RHS;
        case LE:
            return LHS <= RHS;
        case EQ:
            return LHS == RHS;
        case NEQ:
            return LHS == RHS;
    }
}

ComparisonOp negateComparisonOp(ComparisonOp op)
{
    switch(op)
    {
        case GT:
            return LE;
        case GE:
            return LT;
        case LT:
            return GE;
        case LE:
            return GT;
        case EQ:
            return NEQ;
        case NEQ:
            return EQ;
    }
}

enum ExpressionType{PLUS, MINUS, MUL, DIV, MOD, POW, AND, OR};
enum Type {DOUBLE, STRING};

#endif