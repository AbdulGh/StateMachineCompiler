F1_main_0
double LHS;
double RHS;
double retD;
double unique0;
double _2_1_m;
double _2_1_n;
input _2_1_m;
input _2_1_n;
LHS = _2_1_m;
RHS = 0.000000;
jumpif LHS <= RHS F1_main_2;
jump F1_main_4;
end

F0_gcd_0
double _1_0_n;
pop _1_0_n;
double _1_0_m;
pop _1_0_m;
LHS = _1_0_m;
RHS = _1_0_n;
jumpif LHS = RHS F0_gcd_1;
jump F0_gcd_2;
end

F0_gcd_1
retD = _1_0_m;
jump F0_gcd_fin;
end

F0_gcd_10
pop _3_1_nMinusm;
pop _1_0_n;
pop _1_0_m;
unique0 = retD;
retD = unique0;
jump F0_gcd_fin;
end

F0_gcd_2
LHS = _1_0_m;
RHS = _1_0_n;
jumpif LHS > RHS F0_gcd_5;
jump F0_gcd_6;
end

F0_gcd_5
double _3_0_mMinusn;
_3_0_mMinusn = _1_0_m - _1_0_n;
push _1_0_m;
push _1_0_n;
push _3_0_mMinusn;
push state F0_gcd_9;
push _3_0_mMinusn;
push _1_0_n;
jump F0_gcd_0;
end

F0_gcd_6
double _3_1_nMinusm;
_3_1_nMinusm = _1_0_n - _1_0_m;
push _1_0_m;
push _1_0_n;
push _3_1_nMinusm;
push state F0_gcd_10;
push _1_0_m;
push _3_1_nMinusm;
jump F0_gcd_0;
end

F0_gcd_9
pop _3_0_mMinusn;
pop _1_0_n;
pop _1_0_m;
unique0 = retD;
retD = unique0;
jump F0_gcd_fin;
end

F0_gcd_fin
return;
end

F1_main_2
retD = -1.000000;
jump F1_main_fin;
end

F1_main_3
double _2_1_result;
push _2_1_n;
push _2_1_m;
push _2_1_result;
push state F1_main_8;
push _2_1_m;
push _2_1_n;
jump F0_gcd_0;
end

F1_main_4
LHS = _2_1_n;
RHS = 0.000000;
jumpif LHS <= RHS F1_main_2;
jump F1_main_3;
end

F1_main_8
pop _2_1_result;
pop _2_1_m;
pop _2_1_n;
unique0 = retD;
_2_1_result = unique0;
print _2_1_result;
jump F1_main_fin;
end

F1_main_fin
return;
end


