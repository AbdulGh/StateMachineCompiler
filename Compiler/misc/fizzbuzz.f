function main() void
{
    print("Limit?\n>");
    double limit;
    input limit;
    double current = 0;

    while (current < limit)
    {
        if (current % 15 == 0)
        then
            print("FizzBuzz\n");
        else
            if (current % 3 == 0)
            then
                print("Fizz\n");
            else
                if (current % 5 == 0)
                then
                    print("Buzz\n");
                else
                    print(current);
                done
            done
        done

        current = current + 1;
    }
}