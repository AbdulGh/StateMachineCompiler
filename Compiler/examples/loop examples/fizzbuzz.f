function main() void
{
    double limit;
    input limit;
    double current = 1;

    while (current < limit)
    {
        if (current % 15 == 0) double FizzBuzz;
        else
        {
            if (current % 3 == 0) double Fizz;
            else
            {
                if (current % 5 == 0) double Buzz;
                else print(current);
            }
        }
        current = current + 1;
    }
}