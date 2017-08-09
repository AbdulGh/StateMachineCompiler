function main() void
{
    print("Limit?\n>");
    double limit;
    input limit;
    double current = 1;

    while (current < limit)
    {
        if (current % 15 == 0)
        {
            print("FizzBuzz");
        }
        else
        {
            if (current % 3 == 0)
            {
                print("Fizz");
            }
            else
            {
                if (current % 5 == 0)
                {
                    print("Buzz");
                }
                else
                {
                    print(current);
                }
            }
        }
        print("\n");
        current = current + 1;
    }
}