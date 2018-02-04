F0_main_0
double retD;
double _2_0_result;
push _2_0_result;
push state F0_main_1;
push 2;
push 2;
jump F1_ack_0;
end

F0_main_1
pop;
print retD;
return;
end

F1_ack_0
double _1_1_y;
pop _1_1_y;
double _1_1_x;
pop _1_1_x;
jumpif _1_1_x = 0.000000 F1_ack_1;
jump F1_ack_2;
end

F1_ack_1
retD = _1_1_y + 1;
jump F1_ack_fin;
end

F1_ack_10
pop _3_1_yMinus1;
pop _3_1_xMinus1;
pop;
pop _1_1_y;
pop _1_1_x;
push _1_1_x;
push _1_1_y;
push retD;
push _3_1_xMinus1;
push _3_1_yMinus1;
push state F1_ack_11;
push _3_1_xMinus1;
push retD;
jump F1_ack_0;
end

F1_ack_11
pop;
pop;
pop;
pop _1_1_y;
pop _1_1_x;
jump F1_ack_fin;
end

F1_ack_2
jumpif _1_1_y = 0.000000 F1_ack_5;
jump F1_ack_6;
end

F1_ack_5
double _3_0_xMinus1;
_3_0_xMinus1 = _1_1_x - 1;
push _1_1_x;
push _1_1_y;
push _3_0_xMinus1;
push state F1_ack_9;
push _3_0_xMinus1;
push 1;
jump F1_ack_0;
end

F1_ack_6
double _3_1_xMinus1;
_3_1_xMinus1 = _1_1_x - 1;
double _3_1_yMinus1;
_3_1_yMinus1 = _1_1_y - 1;
double _3_1_a2;
push _1_1_x;
push _1_1_y;
push _3_1_a2;
push _3_1_xMinus1;
push _3_1_yMinus1;
push state F1_ack_10;
push _1_1_x;
push _3_1_yMinus1;
jump F1_ack_0;
end

F1_ack_9
pop;
pop _1_1_y;
pop _1_1_x;
jump F1_ack_fin;
end

F1_ack_fin
return;
end
