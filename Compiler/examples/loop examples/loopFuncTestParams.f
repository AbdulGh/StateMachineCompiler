function main() void {call loopheader(0);}

function loopheader(double x) void {if (x < 10) call loopbody(x);}

function loopbody(double x) void
{
	x = x + 1;
	print(x, "\n");
	call loopheader(x);
}
