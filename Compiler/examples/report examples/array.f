function main() void
{
    double[100] a;
    nondet a;

    double i;
    input i;

    if (i > 0)
    {
        while (i < 100 && a[i] >= 0)
        {
            i = i + a[i];
            a[i] = a[i] - i;
        }
    }
}
