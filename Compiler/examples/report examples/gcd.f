function gcd(double m, double n) double
{
	if (m == n) return m;
	else if (m > n)
	{
	    m = m - n;
	    return call gcd(m, n);
	}
	else
	{
	    n = n - m;
	    return call gcd(m, n);
	}
}

function main() void
{
    double m, double n;
    input m; input n;
    if (m <= 0 || n <= 0) return -1;
    double result = call gcd(m, n);
    print(result);
}
