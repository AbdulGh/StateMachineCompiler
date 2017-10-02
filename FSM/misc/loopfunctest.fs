start
double LHS;
double RHS;
string retS;
double retD;
double _0_0_x;
_0_0_x = 0;
push state F_main_1;
LHS = _0_0_x;
RHS = 10.000000;
jumpif LHS < RHS F_loopheader_4;
jump F_loopheader_3;
end

F_loopbody_1
jump F_loopheader_fin;
end

F_loopheader_3
jump F_loopheader_fin;
end

F_loopheader_4
print _0_0_x;
print "\n";
_0_0_x = _0_0_x + 1;
push state F_loopbody_1;
LHS = _0_0_x;
RHS = 10.000000;
jumpif _0_0_x < 10.000000 F_loopheader_4;
jump F_loopheader_3;
end

F_loopheader_fin
return;
end

F_main_1
return;
end