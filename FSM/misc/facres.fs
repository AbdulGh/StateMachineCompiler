start
double LHS;
double RHS;
string retS;
double retD;
double _2_1_in;
input _2_1_in;
double _2_1_facRes;
push _2_1_in;
push _2_1_facRes;
push state F_main_1;
double _1_0_n;
_1_0_n = _2_1_in;
LHS = _1_0_n;
RHS = 0.000000;
jumpif _1_0_n <= 0.000000 F_fac_4;
jump F_fac_3;
end

F_fac_5
pop _3_0_ret;
pop _3_0_arg;
pop _1_0_n;
double unique0;
unique0 = retD;
_3_0_ret = _1_0_n * retD;
retD = _3_0_ret;
jump F_fac_fin;
end

F_main_1
pop _2_1_facRes;
pop _2_1_in;
unique0 = retD;
_2_1_facRes = retD;
print _2_1_facRes;
return;
end

F_fac_4
retD = 1;
jump F_fac_fin;
end

F_fac_fin
return;
end

F_fac_3
double _3_0_arg;
_3_0_arg = _1_0_n - 1;
double _3_0_ret;
push _1_0_n;
push _3_0_arg;
push _3_0_ret;
push state F_fac_5;
double _1_0_n;
_1_0_n = _3_0_arg;
LHS = _1_0_n;
RHS = 0.000000;
jumpif _1_0_n <= 0.000000 F_fac_4;
jump F_fac_3;
end