F0_main_0
double LHS;
double RHS;
double retD;
string retS;
push state F0_main_1;
push 0;
jump F1_loopheader_0;
end

F0_main_1
return;
end

F1_loopheader_0
double _1_1_x;
pop _1_1_x;
LHS = _1_1_x;
RHS = 10.000000;
jumpif _1_1_x < 10.000000 F1_loopheader_4;
jump F1_loopheader_2;
end

F1_loopheader_2
return;
end

F1_loopheader_4
push _1_1_x;
double _1_2_x;
_1_2_x = _1_1_x;
print _1_1_x;
print "\n";
_1_2_x = _1_1_x + 1;
push _1_2_x;
push state F2_loopbody_1;
push _1_2_x;
jump F1_loopheader_0;
end

F2_loopbody_1
pop _1_2_x;
pop _1_1_x;
jump F1_loopheader_2;
end