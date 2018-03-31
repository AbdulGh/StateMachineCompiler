function main() void
{
	double x; input x;
	while (x >= 0) {
		double y = 1;
		while (x > y) y = 2 * y;
		x = x - 1;
	}
}