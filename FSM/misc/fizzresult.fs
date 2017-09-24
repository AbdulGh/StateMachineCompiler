start
double LHS;
double RHS;
string retS;
double retD;
jump F_main_0;
end

F_main_18
print _2_0_current;
print "\n";
_2_0_current = _2_0_current + 1;
LHS = _2_0_current;
RHS = _2_0_limit;
jumpif _2_0_current < _2_0_limit F_main_5;
jump F_main_4;
end

F_main_4
return;
end

F_main_0
print "Limit?\n>";
double _2_0_limit;
input _2_0_limit;
double _2_0_current;
_2_0_current = 1.000000;
LHS = 1.000000;
RHS = _2_0_limit;
jumpif _2_0_limit > 1.000000 F_main_5;
jump F_main_4;
end

F_main_9
print "FizzBuzz";
print "\n";
_2_0_current = _2_0_current + 1;
LHS = _2_0_current;
RHS = _2_0_limit;
jumpif _2_0_current < _2_0_limit F_main_5;
jump F_main_4;
end

F_main_8
LHS = _2_0_current % 3;
RHS = 0.000000;
jumpif LHS = 0.000000 F_main_14;
jump F_main_13;
end

F_main_19
print "Buzz";
print "\n";
_2_0_current = _2_0_current + 1;
LHS = _2_0_current;
RHS = _2_0_limit;
jumpif _2_0_current < _2_0_limit F_main_5;
jump F_main_4;
end

F_main_5
LHS = _2_0_current % 15;
RHS = 0.000000;
jumpif LHS = 0.000000 F_main_9;
jump F_main_8;
end

F_main_13
LHS = _2_0_current % 5;
RHS = 0.000000;
jumpif LHS = 0.000000 F_main_19;
jump F_main_18;
end

F_main_14
print "Fizz";
print "\n";
_2_0_current = _2_0_current + 1;
LHS = _2_0_current;
RHS = _2_0_limit;
jumpif _2_0_current < _2_0_limit F_main_5;
jump F_main_4;
end