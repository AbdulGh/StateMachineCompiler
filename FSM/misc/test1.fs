double counter;

states

one
print "Counter is equal to ";
printvar counter;
print "\n";
counter = 5;
print "Counter is now equal to ";
printvar counter;
print "\n";
jumpif counter > 4 two;
print "Didn't work\n";
end

two
print "Holy moly it works\n";
end