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
jumpif _2_0_current < _2_0_limit F0_main_5;
return;
end

F0_main_11
print "Fizz";
print "\n";
_2_0_current = _2_0_current + 1;
jump F0_main_1;
end

F0_main_13
LHS = _2_0_current % 5;
jumpif LHS = 0.000000 F0_main_16;
jump F0_main_18;
end

F0_main_16
print "Buzz";
print "\n";
_2_0_current = _2_0_current + 1;
jump F0_main_1;
end

F0_main_18
print _2_0_current;
print "\n";
_2_0_current = _2_0_current + 1;
jump F0_main_1;
end

F0_main_5
LHS = _2_0_current % 15;
jumpif LHS = 0.000000 F0_main_6;
jump F0_main_7;
end

F0_main_6
print "FizzBuzz";
print "\n";
_2_0_current = _2_0_current + 1;
jump F0_main_1;
end

F0_main_7
LHS = _2_0_current % 3;
jumpif LHS = 0.000000 F0_main_11;
jump F0_main_13;
end

F0_main_0
print "Limit?\n>";
double _2_0_current;
double _2_0_limit;
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
jump F0_main_18;
end

F0_main_16
print "Buzz";
print "\n";
jump F0_main_1;
end

F0_main_18
print 1.000000;
print "\n";
jump F0_main_1;
end

F0_main_5
jump F0_main_7;
end

F0_main_6
print "FizzBuzz";
print "\n";
_2_0_current = _2_0_current + 1;
jump F0_main_1;
end

F0_main_7
jump F0_main_13;
end