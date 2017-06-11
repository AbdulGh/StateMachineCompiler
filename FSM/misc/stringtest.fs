start
string LHS;
print "LHS string:\n";
input LHS;
string RHS;
print "RHS string:\n";
input RHS;
jumpif LHS < RHS less;
jumpif LHS = RHS equal;
jumpif LHS > RHS greater;
end

less
print "Less\n";
jump start;
end

equal
print "Equal\n";
jump start;
end

greater
print "Greater\n";
jump start;
end