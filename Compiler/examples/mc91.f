function main() void
{
    double n;
    input n;
    double result = call mc91(n);
    print(result);
}

function mc91(double n) double
{
    if (n > 100) return n-10;
    else
    {
        n = n + 11;
        double m1 = call mc91(n);
        double m2 = call mc91(m1);
        return m2;
    }
}