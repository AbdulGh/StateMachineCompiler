function evaluateLinearFunction(double x, double a, double b) double
{
    double y = a * x + b;
    return y;
}

function main() void
{
    double a;
    double x;
    double b;
    print("a: ");
    input a;
    print("x: ");
    input x;
    print("b: ");
    input b;
    double y = call evaluateLinearFunction(x, a, b);
    print(y);
    return;
}