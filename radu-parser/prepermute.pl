#!/usr/usc/bin/perl -w

while(my $line=<>){
    $line =~ /^([0-9]+) (.*?)$/;
    my $n = $1; my $cn = $n;
    my @line = split " ", $2;
    my $cline = "";
    for(my $i=0; $i<2*$n; $i+=2){
	if( $line[$i+1] eq "``" || $line[$i+1] eq "''" || $line[$i+1] eq "." ){
	    $cn--; next;
	}
	$cline .= " $line[$i]_$line[$i+1]";
    }
    print "$cn$cline\n";
}
