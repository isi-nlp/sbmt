#!/usr/usc/bin/perl

# transform input for parsing from Ratnparkhi style to Collins style

while($line = <>){
    chomp($line);
    @line = split " ", $line;
    $n = scalar(@line);
    print "$n ";
    for($i=0; $i<$n; $i++){
	$line[$i] =~ s/^(.*)\_(.*?)$/$1 $2/;
	print $line[$i]." ";
    }
    print "\n";
}
