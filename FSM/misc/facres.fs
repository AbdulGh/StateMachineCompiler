F1_main_0
double LHS;
double RHS;
string retS;
double retD;
double _2_1_in;
input _2_1_in;
double _2_1_facRes;
push _2_1_in;
push _2_1_facRes;
push state F1_main_1;
push _2_1_in;
double _1_0_n;
pop _1_0_n;
LHS = _1_0_n;
RHS = 0.000000;
jumpif LHS <= RHS F0_fac_1;
jump F0_fac_2;
end

F1_main_1
pop _2_1_facRes;
pop _2_1_in;
unique0 = retD;
_2_1_facRes = unique0;
print _2_1_facRes;
return;
end

F0_fac_5
pop _3_0_ret;
pop _3_0_arg;
pop _1_0_n;
double unique0;
unique0 = retD;
_3_0_ret = _1_0_n * unique0;
retD = _3_0_ret;
jump F0_fac_fin;
end

F0_fac_fin
return;
end

F0_fac_1
retD = 1;
jump F0_fac_fin;
end

F0_fac_2
double _3_0_arg;
_3_0_arg = _1_0_n - 1;
double _3_0_ret;
push _1_0_n;
push _3_0_arg;
push _3_0_ret;
push state F0_fac_5;
push _3_0_arg;
double _1_0_n;
pop _1_0_n;
LHS = _1_0_n;
RHS = 0.000000;
jumpif LHS <= RHS F0_fac_1;
jump F0_fac_2;
end
