start
double LHS;
double RHS;
string retS;
double retD;
jump F_main_0;
end

F_main_1
double unique0;
unique0 = retD;
_2_1_y = unique0;
print _2_1_y;
return;
end

F_main_0
double _2_1_a;
double _2_1_x;
double _2_1_b;
print "a: ";
input _2_1_a;
print "x: ";
input _2_1_x;
print "b: ";
input _2_1_b;
double _2_1_y;
push state F_main_1;
push _2_1_x;
push _2_1_a;
push _2_1_b;
double _1_0_b;
pop _1_0_b;
double _1_0_a;
pop _1_0_a;
double _1_0_x;
pop _1_0_x;
double _2_0_y;
_2_0_y = _1_0_a * _1_0_x;
_2_0_y = _2_0_y + _1_0_b;
retD = _2_0_y;
return;
end