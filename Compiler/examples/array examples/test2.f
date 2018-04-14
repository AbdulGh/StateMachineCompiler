function main() void
{
    double[100] array;
    double x = 0;
    while (x <= 100)
    {
        array[x] = 2;
        x = x + 1;
    }
    print(array[5]);
}