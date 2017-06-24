start
double retD;
string retS;
double LHS;
double RHS;
jump F_main_0;
end

funFinish
jump pop;
end

F_main_0
print "The condition will be (i < 4 && i != 3 || i > 12 && i < 19)\n";
double _0_2_i;
input _0_2_i;
LHS = _0_2_i;
RHS = 4;
jumpif LHS < RHS F_main_4;
jump F_main_3;
end

F_main_4
LHS = _0_2_i;
RHS = 3;
jumpif LHS != RHS F_main_5;
jump F_main_3;
end

F_main_5
jump F_main_1;
end

F_main_3
LHS = _0_2_i;
RHS = 12;
jumpif LHS > RHS F_main_7;
jump F_main_6;
end

F_main_7
LHS = _0_2_i;
RHS = 19;
jumpif LHS < RHS F_main_8;
jump F_main_6;
end

F_main_8
jump F_main_1;
end

F_main_6
jump F_main_2;
end

F_main_1
print "It met the condition\n";
jump F_main_9;
end

F_main_2
print "Failed the condition\n";
jump F_main_9;
end

F_main_9
print "Calling main...\n";
push state F_main_10;
jump F_main_0;
end

F_main_10
jump funFinish;
end