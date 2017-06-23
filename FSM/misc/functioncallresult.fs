start
double retD;
string retS;
string _0_0_message;
_0_0_message = "Hello world\n";
jump F_main_0;
end

funFinish
jump pop;
end

F_main_0
push state F_main_1;
jump F_check_0;
end

F_main_1
jump funFinish;
end

F_check_0
print _0_0_message;
_0_0_message = "testing overwriting\n";
print _0_0_message;
string _3_2_message;
_3_2_message = "testing scopes\n";
print _3_2_message;
print "getReturnVarWorkingPercentage: ";
double _3_2_d;
push state F_check_1;
jump F_getReturnVarWorkingPercentage_0;
end

F_check_1
double unique0;
unique0 = retD;
_3_2_d = unique0;
print _3_2_d;
print "%\ngetReturnLiteralWorkingAmount: ";
string _3_2_message2;
push state F_check_2;
jump F_getReturnLiteralWorkingAmount_0;
end

F_check_2
_3_2_message2 = retS;
print _3_2_message2;
print "\nreturnLiteral correctness percentage: ";
push state F_check_3;
push 100;
jump F_returnArgument_0;
end

F_check_3
unique0 = retD;
_3_2_d = unique0;
print _3_2_d;
print "%\nreturnDouble correcness percentage * 0.5: ";
_3_2_d = 50;
double _3_2_d2;
push state F_check_4;
push _3_2_d;
jump F_returnArgument_0;
end

F_check_4
unique0 = retD;
_3_2_d2 = unique0;
print _3_2_d;
jump funFinish;
end

F_returnArgument_0
double _2_1_d;
pop _2_1_d;
retD = _2_1_d;
jump funFinish;
end

F_getReturnVarWorkingPercentage_0
double _0_2_omg;
_0_2_omg = 100;
retD = _0_2_omg;
jump funFinish;
end

F_getReturnLiteralWorkingAmount_0
retS = "One hundred percent";
jump funFinish;
end