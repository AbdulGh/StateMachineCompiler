F0_main_0
double retD;
double _2_0_result;
push _2_0_result;
push state F0_main_1;
push 2;
push 2;
double _3_0_xMinus1;
double unique0;
jump F1_ack_0;
end

F0_main_1
pop _2_0_result;
unique0 = retD;
print unique0;
return;
end

F1_ack_0
double _1_1_y;
pop _1_1_y;
double _1_1_x;
pop _1_1_x;
jumpif _1_1_x <= 0.000000 F1_ack_1;
jump F1_ack_2;
end

F1_ack_1
retD = _1_1_y + 1.000000;
jump F1_ack_fin;
end

F1_ack_10
pop _3_1_a2;
pop _3_1_yMinus1;
pop _3_1_xMinus1;
pop _1_1_y;
pop _1_1_x;
unique0 = retD;
push _1_1_x;
push _1_1_y;
push _3_1_xMinus1;
push _3_1_yMinus1;
push unique0;
push state F1_ack_11;
push _3_1_xMinus1;
push unique0;
jump F1_ack_0;
end

F1_ack_11
pop _3_1_a2;
pop _3_1_yMinus1;
pop _3_1_xMinus1;
pop _1_1_y;
pop _1_1_x;
unique0 = retD;
retD = unique0;
jump F1_ack_fin;
end

F1_ack_2
jumpif _1_1_y <= 0.000000 F1_ack_5;
jump F1_ack_6;
end

F1_ack_5
push _1_1_x;
push _1_1_y;
push _1_1_x;
push state F1_ack_9;
push _1_1_x;
push 1;
jump F1_ack_0;
end

F1_ack_6
double _3_1_xMinus1;
_3_1_xMinus1 = _1_1_x - 1.000000;
double _3_1_yMinus1;
_3_1_yMinus1 = _1_1_y - 1.000000;
double _3_1_a2;
push _1_1_x;
push _1_1_y;
push _3_1_xMinus1;
push _3_1_yMinus1;
push _3_1_a2;
push state F1_ack_10;
push _1_1_x;
push _3_1_yMinus1;
jump F1_ack_0;
end

F1_ack_9
pop _3_0_xMinus1;
pop _1_1_y;
pop _1_1_x;
unique0 = retD;
retD = unique0;
jump F1_ack_fin;
end

F1_ack_fin
return;
end