function getstr(int integer) string
{
	string str = "good\n";
	if (integer < 5) return str;
	return "bad\n";
}

function main() int
{
    string str = getstr(4);
    print(str);
    str = getstr(5);
    print(str);
    return 0;
}