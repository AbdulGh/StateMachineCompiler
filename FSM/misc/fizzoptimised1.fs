start
double retD;
string retS;
double LHS;
double RHS;
jump F_main_0;
end

funFinish
jump pop;
end

F_main_0
print "Limit?\n>";
double _0_2_limit;
input _0_2_limit;
double _0_2_current;
double temp0;
temp0 = 1;
_0_2_current = temp0;
jump F_main_1;
end

F_main_1
temp0 = _0_2_current;
LHS = temp0;
temp0 = _0_2_limit;
RHS = temp0;
jumpif LHS < RHS F_main_2;
jump funFinish;
end

F_main_2
temp0 = _0_2_current;
temp0 = temp0 % 15;
LHS = temp0;
temp0 = 0;
RHS = temp0;
jumpif LHS = RHS F_main_6;
jump F_main_7;
end

F_main_6
print "FizzBuzz";
jump F_main_10;
end

F_main_7
temp0 = _0_2_current;
temp0 = temp0 % 3;
LHS = temp0;
temp0 = 0;
RHS = temp0;
jumpif LHS = RHS F_main_11;
jump F_main_12;
end

F_main_11
print "Fizz";
jump F_main_10;
end

F_main_12
temp0 = _0_2_current;
temp0 = temp0 % 5;
LHS = temp0;
temp0 = 0;
RHS = temp0;
jumpif LHS = RHS F_main_16;
jump F_main_17;
end

F_main_16
print "Buzz";
jump F_main_10;
end

F_main_17
print _0_2_current;
jump F_main_10;
end

F_main_10
print "\n";
temp0 = _0_2_current;
temp0 = temp0 + 1;
_0_2_current = temp0;
jump F_main_1;
end