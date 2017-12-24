F0_main_0
double _2_0_x;
input _2_0_x;
push _2_0_x;
push state F0_main_1;
push _2_0_x;
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
jumpif _1_1_x = 0.000000 F1_isEven_1;
jumpif _1_1_x = 1.000000 F1_isEven_6;
_1_1_x = _1_1_x - 1;
jumpif _1_1_x = 1.000000 F2_isEvenNegated_1;
jumpif _1_1_x = 0.000000 F2_isEvenNegated_6;
jump F2_isEvenNegated_7;
end

F1_isEven_1
print "It's even\n";
return;
end

F1_isEven_6
print "It's odd\n";
return;
end

F2_isEvenNegated_1
print "It's even\n";
return;
end

F2_isEvenNegated_11
pop;
return;
end

F2_isEvenNegated_6
print "It's odd\n";
return;
end

F2_isEvenNegated_7
_1_2_x = _1_1_x - 1;
push _1_2_x;
push state F2_isEvenNegated_11;
push _1_2_x;
jump F1_isEven_0;
end