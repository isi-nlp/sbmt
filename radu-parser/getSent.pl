#!/usr/usc/bin/perl -w

while($line=<>){
    chomp($line);
    @line = split " ", $line;
    for($i=1; $i<=$#line; $i+=2){
	print lc($line[$i])." ";
    }
    print "\n";
}
