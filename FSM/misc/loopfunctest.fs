F0_main_0
double LHS;
double RHS;
string retS;
double retD;
double _0_0_x;
_0_0_x = 0;
push state F0_main_1;
jump F1_loopheader_0;
end

F0_main_1
jump F0_main_fin;
end

F0_main_fin
return;
end

F1_loopheader_0
LHS = _0_0_x;
RHS = 10.000000;
jumpif LHS < RHS F1_loopheader_4;
jump F1_loopheader_3;
end

F1_loopheader_1
push state F1_loopheader_5;
jump F2_loopbody_0;
end

F1_loopheader_2
jump F1_loopheader_fin;
end

F1_loopheader_3
jump F1_loopheader_2;
end

F1_loopheader_4
jump F1_loopheader_1;
end

F1_loopheader_5
jump F1_loopheader_2;
end

F1_loopheader_fin
return;
end

F2_loopbody_0
print _0_0_x;
print "\n";
_0_0_x = _0_0_x + 1;
push state F2_loopbody_1;
jump F1_loopheader_0;
end

F2_loopbody_1
jump F2_loopbody_fin;
end

F2_loopbody_fin
return;
end
