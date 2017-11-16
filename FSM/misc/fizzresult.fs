F0_main_0
print "Limit?\n>";
double _2_0_limit;
input _2_0_limit;
double _2_0_current;
_2_0_current = 1.000000;
double LHS;
jump F0_main_1;
end

F0_main_1
jumpif 1.000000 < _2_0_limit F0_main_5;
return;
end

F0_main_11
print "Fizz";
print "\n";
_2_0_current = _2_0_current + 1;
jump F0_main_1;
end

F0_main_13
LHS = 1.000000;
jump F0_main_18;
end

F0_main_16
print "Buzz";
print "\n";
_2_0_current = 2.000000;
jump F0_main_1;
end

F0_main_18
print 1.000000;
print "\n";
_2_0_current = 2.000000;
jump F0_main_1;
end

F0_main_5
LHS = 1.000000;
jump F0_main_7;
end

F0_main_6
print "FizzBuzz";
print "\n";
_2_0_current = _2_0_current + 1;
jump F0_main_1;
end

F0_main_7
LHS = 1.000000;
jump F0_main_13;
end