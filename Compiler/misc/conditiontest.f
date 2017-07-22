function main() void
{
    print("The condition will be (i < 4 && i != 3 || i > 12 && i < 19)\n");
    double i;
    input i;
    if (i < 4 && i != 3 || i > 12 && i < 19) then print("It met the condition\n");
    else print("Failed the condition\n"); done
    print("Calling main...\n");
    call main();
}