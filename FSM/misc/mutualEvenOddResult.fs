F0_main_0
double _2_0_x;
input _2_0_x;
push _2_0_x;
push state F0_main_1;
push _2_0_x;
double LHS;
double RHS;
double _1_2_x;
jump F1_isEven_0;
end

F0_main_1
pop;
return;
end

F1_isEven_0
double _1_1_x;
pop _1_1_x;
LHS = _1_1_x;
RHS = 0.000000;
jumpif LHS = RHS F1_isEven_1;
jump F1_isEven_2;
end

F1_isEven_1
print "It's even\n";
jump F1_isEven_fin;
end

F1_isEven_2
LHS = _1_1_x;
RHS = 1.000000;
jumpif LHS = RHS F1_isEven_6;
jump F1_isEven_7;
end

F1_isEven_6
print "It's odd\n";
jump F1_isEven_fin;
end

F1_isEven_7
_1_1_x = _1_1_x - 1;
LHS = _1_1_x;
RHS = 1.000000;
jumpif LHS = RHS F2_isEvenNegated_1;
jump F2_isEvenNegated_2;
end

F1_isEven_fin
return;
end

F2_isEvenNegated_1
print "It's even\n";
jump F1_isEven_fin;
end

F2_isEvenNegated_11
pop;
jump F1_isEven_fin;
end

F2_isEvenNegated_2
LHS = _1_1_x;
RHS = 0.000000;
jumpif LHS = RHS F2_isEvenNegated_6;
jump F2_isEvenNegated_7;
end

F2_isEvenNegated_6
print "It's odd";
jump F1_isEven_fin;
end

F2_isEvenNegated_7
_1_2_x = _1_1_x - 1;
push _1_2_x;
push state F2_isEvenNegated_11;
push _1_2_x;
jump F1_isEven_0;
end