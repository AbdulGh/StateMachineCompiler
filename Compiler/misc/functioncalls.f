string message = "Hello world\n";

function getReturnVarWorkingPercentage() double
{
    double omg = 100;
    return omg;
}

function getReturnLiteralWorkingAmount() string
{
    return "One hundred percent";
}

function returnArgument(double d) double
{
    return d;
}

function check() void
{
    print(message);
    message = "testing overwriting\n";
    print(message);
    string message = "testing scopes\n";
    print(message);

    print("getReturnVarWorkingPercentage: ");
    double d = call getReturnVarWorkingPercentage();
    print(d);

    print("%\ngetReturnLiteralWorkingAmount: ");
    string message2 = call getReturnLiteralWorkingAmount();
    print(message2);

    print("\nreturnLiteral correctness percentage: ");
    d = call returnArgument(100);
    print(d);

    print("%\nreturnDouble correcness percentage * 0.5: ");
    d = 50;
    double d2 = call returnArgument(d);
    print(d);
}

function main() void
{
    call check();
}