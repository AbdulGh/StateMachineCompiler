function main() void
{
	double x = 99;
	double[100] array;
	nondet array;
	while (x > 0)
	{
		double y = 0;
		while (y < x)
		{
			double yPlus1 = y + 1;
			if (array[y] > array[yPlus1])
			{
				double t = array[y];
				array[y] = array[yPlus1];
				array[yPlus1] = t;
			}
			y = yPlus1;
		}
		x = x - 1;
	}
}
