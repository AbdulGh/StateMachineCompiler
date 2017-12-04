double x = 0;

function main() void {call loopheader();}

function loopheader() void {if (x < 10) call loopbody();}

function loopbody() void
{
	print(x, "\n");
	x = x + 1;
	call loopheader();
}
