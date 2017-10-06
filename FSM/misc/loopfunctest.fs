F0_main_0
double LHS;
double RHS;
string retS;
double retD;
double _0_0_x;
_0_0_x = 0;
push state F0_main_fin;
LHS = 0;
RHS = 10.000000;
jump F1_loopheader_4;
end

F0_main_fin
return;
end

F1_loopheader_4
push state F1_loopheader_fin;
print _0_0_x;
print "\n";
_0_0_x = _0_0_x + 1;
push state F2_loopbody_fin;
LHS = _0_0_x;
RHS = 10.000000;
jumpif _0_0_x < 10.000000 F1_loopheader_4;
jump F1_loopheader_fin;
end

F1_loopheader_fin
return;
end

F2_loopbody_fin
return;
end