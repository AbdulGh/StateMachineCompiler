function main() void
{
    double x;
    input x;
    if (x >= 50) call over50(x);
}

function over50(double x) void
{
    if (x < 50) print("bad\n");
    else print("good\n");
}