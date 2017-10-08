F0_main_0
double LHS;
double RHS;
string retS;
double retD;
double _0_0_x;
_0_0_x = 0;
push state F0_main_fin;
LHS = _0_0_x;
RHS = 10.000000;
jumpif LHS < RHS F1_loopheader_4;
return;
end

F2_loopbody_fin
return;
end

F1_loopheader_4
print _0_0_x;
print "\n";
_0_0_x = _0_0_x + 1;
push state F2_loopbody_fin;
LHS = _0_0_x;
RHS = 10.000000;
jumpif LHS < RHS F1_loopheader_4;
return;
end

F0_main_fin
return;
end
