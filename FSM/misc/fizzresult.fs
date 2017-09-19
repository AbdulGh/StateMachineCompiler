start
double LHS;
double RHS;
string retS;
double retD;
jump F_main_0;
end

F_main_17
print _2_0_current;
print "\n";
_2_0_current = _2_0_current + 1;
LHS = _2_0_current;
RHS = _2_0_limit;
jumpif _2_0_current < _2_0_limit F_main_4;
jump F_main_3;
end

F_main_18
print "Buzz";
print "\n";
_2_0_current = _2_0_current + 1;
LHS = _2_0_current;
RHS = _2_0_limit;
jumpif _2_0_current < _2_0_limit F_main_4;
jump F_main_3;
end

F_main_4
LHS = _2_0_current % 15;
RHS = 0.000000;
jumpif LHS = 0.000000 F_main_8;
jump F_main_7;
end

F_main_8
print "FizzBuzz";
print "\n";
_2_0_current = _2_0_current + 1;
LHS = _2_0_current;
RHS = _2_0_limit;
jumpif _2_0_current < _2_0_limit F_main_4;
jump F_main_3;
end

F_main_0
LHS = _2_0_current;
RHS = _2_0_limit;
jumpif _2_0_current < _2_0_limit F_main_4;
jump F_main_3;
end

F_main_7
LHS = _2_0_current % 3;
RHS = 0.000000;
jumpif LHS = 0.000000 F_main_13;
jump F_main_12;
end

F_main_12
LHS = _2_0_current % 5;
RHS = 0.000000;
jumpif LHS = 0.000000 F_main_18;
jump F_main_17;
end

F_main_3
return;
end

F_main_13
print "Fizz";
print "\n";
_2_0_current = _2_0_current + 1;
LHS = _2_0_current;
RHS = _2_0_limit;
jumpif _2_0_current < _2_0_limit F_main_4;
jump F_main_3;
end