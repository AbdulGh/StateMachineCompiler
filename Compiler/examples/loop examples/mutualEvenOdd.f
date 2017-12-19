double x;

function main() void
{
    input x;
    call isEven();
}

function isEven() void
{
    if (x == 0) print("It's even\n");
    else if (x == 1) print("It's odd\n");
    else
    {
        x = x - 1;
        call isEvenNegated();
    }
}

function isEvenNegated() void
{
    if (x == 1) print("It's even\n");
    else if (x == 0) print("It's odd\n");
    else
    {
        x = x - 1;
        call isEven();
    }
}