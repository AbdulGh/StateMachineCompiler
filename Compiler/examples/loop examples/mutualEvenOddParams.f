function main() void
{
    double x;
    input x;
    call isEven(x);
}

function isEven(double x) void
{
    if (x == 0) print("It's even\n");
    else if (x == 1) print("It's odd\n");
    else
    {
        x = x - 1;
        call isEvenNegated(x);
    }
}

function isEvenNegated(double x) void
{
    if (x == 1) print("It's even\n");
    else if (x == 0) print("It's odd\n");
    else
    {
        x = x - 1;
        call isEven(x);
    }
}