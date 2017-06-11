double test;
string two;
states

start
two = "two";
test = 1;
push 3;
push two;
push test;
two = "bad";
test = 6;
pop test;
printvar test;
print "\n";
pop two;
printvar two;
print "\n";
pop test;
printvar test;
print "\n";
end