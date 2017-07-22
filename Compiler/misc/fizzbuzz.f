function main() void
{
    print("Limit?\n>");
    double limit;
    input limit;
    double current = 1;

    while (current < limit)
    {
        if (current % 15 == 0)
        then
            print("FizzBuzz");
        else
            if (current % 3 == 0)
            then
                print("Fizz");
            else
                if (current % 5 == 0)
                then
                    print("Buzz");
                else
                    print(current);
                done
            done
        done
        print("\n");
        current = current + 1;
    }
}