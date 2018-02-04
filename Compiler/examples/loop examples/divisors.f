function main() void
{
    double divisors = 0;
    double n;
    input n;
    double i = 1;
    while (i < n)
    {
    	if (n % i == 0) divisors = divisors + 1;
    	else i = i + 1;
    }
}