function fac(double n) double
{
    if (n <= 0) return 1;
    else
    {
        double arg = n-1;
        double ret = n * call fac(arg);
        return ret;
    }
}

function main() void
{
    double in;
    input in;
    double facRes = call fac(in);
    print(facRes);
}