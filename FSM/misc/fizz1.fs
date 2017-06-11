double counter;
double upper;
double mod15;
double mod3;
double mod5;

states

start
print "Limit: ";
input upper;
jumpif upper < 1 err;
jump loop;
end

err
print "Limit must be at least 1\n";
jump start;
end

loop
counter = counter + 1;
jumpif counter > upper fin;
mod15 = counter % 15;
jumpif mod15 = 0 fizzbuzz;
mod3 = counter % 3;
jumpif mod3 = 0 fizz;
mod5 = counter % 5;
jumpif mod5 = 0 buzz;
printvar counter;
print "\n";
jump loop;
end

fizzbuzz
print "FizzBuzz\n";
jump loop;
end

fizz
print "Fizz\n";
jump loop;
end

buzz
print "Buzz\n";
jump loop;
end

fin
end