#!/usr/usc/bin/perl

$start = $ARGV[0];
$end = $ARGV[1];
open(F, "$ARGV[2]") or die;

$N = 1; 
while($line=<F>){ 
    $line =~ /sent=(.*?) /; 
    $cN = $1; 
    if( $cN!=$N ){ 
	for($i=$N; $i<$cN; $i++){
	    print "==".$i."==\n";  
	}
	$N = $cN;
    } 
    $line=~/$start(.*?)$end/;
    $extract = $1;
    $extract =~ s/{{{//; $extract =~ s/}}}//; 
    if( $start eq "tree=" ){ $extract = "(TOP $extract)"; }
    print "$extract\n";
} 
print "==".$N."==\n";
