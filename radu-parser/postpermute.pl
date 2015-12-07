#!/usr/usc/bin/perl -w

while(my $line=<>){
    $line =~ /^([0-9]+) (.*?)$/;
    my @line = split " ", $2;
    for($i=0; $i<=$#line; $i++){
	$line[$i] =~ /(.*?)\_(.*?)/;
	print "$1 ";
    }
    print "\n";
}
