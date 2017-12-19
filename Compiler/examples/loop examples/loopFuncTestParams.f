function main() void {call loopheader(0);}

function loopheader(double x) void {if (x < 10) call loopbody(x);}

function loopbody(double x) void
{
	print(x, "\n");
	x = x + 1;
	call loopheader(x);
}
