#!/usr/usc/bin/perl -w

open(I, $ARGV[0]) or die; # initial n-best input
open(P, $ARGV[1]) or die; # probs

$cnt = 1;
while( $iline = <I> ){
    chomp($iline);
    $pline = <P>; chomp($pline);
    if( $iline =~ /^(.*?) (.*)$/ ){ 
	$stamp = $1; $line = $2;
    }
    else{ die "Error in iline at line $cnt: $iline\n";}
    if( $pline =~ /^(.*?) / ){
	$prob = $1;
    }
    elsif( $pline eq "-3000" ){
	$prob = "-inf";
    }
    else{ die "Error in pline at line $cnt: $pline\n";}
    print "$stamp $prob $line\n";
    $cnt++;
}
