first
double t;
t = 5;
push state three;
push state two;
push state one;
jumpif t != 5 conTest;
print "zero\n";
jump pop;
end

one
print "one\n";
jump pop;
end

two
print "two\n";
jump pop;
end

three
print "three\n";
end

conTest
push state conSuccess;
jumpif t = 4 pop;
end

conSuccess
print "Good stuff\n";
end