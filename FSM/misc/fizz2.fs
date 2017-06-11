start
print "Limit: ";
double upper;
input upper;
jumpif upper < 1 err;
double counter;
counter = 0;
jump loop;
end

err
print "Limit must be at least 1\n";
jump start;
end

loop
counter = counter + 1;
double temp;
temp = counter % 15;
jumpif temp = 0 fizzbuzz;
temp = counter % 3;
jumpif temp = 0 fizz;
temp = counter % 5;
jumpif temp = 0 buzz;
print counter;
print "\n";
jumpif counter < upper loop;
end

fizzbuzz
print "FizzBuzz\n";
jumpif counter < upper loop;
end

fizz
print "Fizz\n";
jumpif counter < upper loop;
end

buzz
print "Buzz\n";
jumpif counter < upper loop;
end