function getstr(double integer) string
{
	string str = "good\n";
	if (integer < 5) return str;
	return "bad\n";
}

function main() double
{
    string str = call getstr(4);
    print(str);
    str = call getstr(5);
    print(str);
    return 0;
}