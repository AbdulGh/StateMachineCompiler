function main() void
{
    double x;
    input x;
    while (x > 0)
    {
        if (x % 2 == 0) print(x, " is even\n");
        else print(x, " is odd\n");
        x = x + 1;
    }
}