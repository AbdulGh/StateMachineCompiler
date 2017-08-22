start
double retD;
string retS;
double LHS;
double RHS;
jump F_main_0;
end

F_main_1
double unique0;
unique0 = retD;
_2_1_d_y = unique0;
print _2_1_d_y;
return;
end

F_evaluateLinearFunction_0
double _1_0_d_b;
pop _1_0_d_b;
double _1_0_d_a;
pop _1_0_d_a;
double _1_0_s_x;
pop _1_0_s_x;
double _2_0_d_y;
_2_0_d_y = _1_0_d_a * _1_0_s_x;
_2_0_d_y = _2_0_d_y + _1_0_d_b;
retD = _2_0_d_y;
return;
end

F_main_0
double _2_1_d_a;
double _2_1_d_x;
double _2_1_d_b;
print "a: ";
input _2_1_d_a;
print "x: ";
input _2_1_d_x;
print "b: ";
input _2_1_d_b;
double _2_1_d_y;
push state F_main_1;
push _2_1_d_x;
push _2_1_d_a;
push _2_1_d_b;
jump F_evaluateLinearFunction_0;
end