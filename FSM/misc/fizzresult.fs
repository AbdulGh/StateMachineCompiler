start
double retD;
string retS;
double LHS;
double RHS;
jump F_main_0;
end

F_main_17
print _0_2_current;
jump F_main_10;
end

F_main_1
LHS = _0_2_current;
RHS = _0_2_limit;
jumpif LHS < RHS F_main_2;
jump pop;
end

F_main_2
LHS = _0_2_current % 15;
RHS = 0;
jumpif LHS = RHS F_main_6;
jump F_main_7;
end

F_main_0
print "Limit?\n>";
double _0_2_limit;
input _0_2_limit;
double _0_2_current;
_0_2_current = 1;
jump F_main_1;
end

F_main_16
print "Buzz";
jump F_main_10;
end

F_main_6
print "FizzBuzz";
jump F_main_10;
end

F_main_12
LHS = _0_2_current % 5;
RHS = 0;
jumpif LHS = RHS F_main_16;
jump F_main_17;
end

F_main_7
LHS = _0_2_current % 3;
RHS = 0;
jumpif LHS = RHS F_main_11;
jump F_main_12;
end

F_main_10
print "\n";
_0_2_current = _0_2_current + 1;
jump F_main_1;
end

F_main_11
print "Fizz";
jump F_main_10;
end