function main() void
{
    double result = call ack(2,2);
    print(result);
}

function ack(double x, double y) double
{
    if (x <= 0) return y+1;
    else if (y <= 0)
    {
        double xMinus1 = x;
        return call ack(xMinus1, 1);
    }
    else
    {
        double xMinus1 = x-1;
        double yMinus1 = y-1;
        double a2 = call ack(x, yMinus1);
        return call ack(xMinus1, a2);
    }
}
