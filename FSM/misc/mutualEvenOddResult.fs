F0_main_0
double _0_0_x;
input _0_0_x;
push state F0_main_1;
double LHS;
double RHS;
jump F1_isEven_0;
end

F0_main_1
return;
end

F1_isEven_0
LHS = _0_0_x;
RHS = 0.000000;
jumpif LHS = RHS F1_isEven_1;
jump F1_isEven_3;
end

F1_isEven_1
print "It's even\n";
jump F1_isEven_fin;
end

F1_isEven_3
LHS = _0_0_x;
RHS = 1.000000;
jumpif LHS = RHS F1_isEven_6;
jump F1_isEven_8;
end

F1_isEven_6
print "It's odd\n";
jump F1_isEven_fin;
end

F1_isEven_8
_0_0_x = _0_0_x - 1;
push state F1_isEven_fin;
LHS = _0_0_x;
RHS = 1.000000;
jumpif LHS = RHS F2_isEvenNegated_4;
jump F2_isEvenNegated_3;
end

F1_isEven_fin
return;
end

F2_isEvenNegated_3
LHS = _0_0_x;
RHS = 0.000000;
jumpif LHS = RHS F2_isEvenNegated_9;
jump F2_isEvenNegated_8;
end

F2_isEvenNegated_4
print "It's even\n";
jump F2_isEvenNegated_fin;
end

F2_isEvenNegated_8
_0_0_x = _0_0_x - 1;
push state F2_isEvenNegated_fin;
jump F1_isEven_0;
end

F2_isEvenNegated_9
print "It's odd";
jump F2_isEvenNegated_fin;
end

F2_isEvenNegated_fin
return;
end