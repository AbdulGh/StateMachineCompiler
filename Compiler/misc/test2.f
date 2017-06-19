double globalT1 = 4;

function getstr(double integer) string
{
	string str = "good\n";
	if (integer < 5) return str;
	return "bad\n";
}

string globalT2;

function main() double
{
    string str = call getstr(4);
    print(str);
    str = call getstr(5);
    print(str);
    return 0;
}

string globalT3 = "wow";