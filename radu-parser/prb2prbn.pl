#!/usr/bin/perl -w

# Takes a PRB file and a LBL file (labels for nbest list)
# and inserts ==n== boundaries when the label changes

open(P, $ARGV[0]) or die;
open(L, $ARGV[1]) or die;

$plabel = 0;
while( $pline=<P> ){
    $label = <L>;
    chomp($label);
    if( $label eq $plabel ){
	print $pline;
    }
    else{
	print "==$plabel==\n";
	$plabel = $label;
	print $pline;
    }
}
print "==$plabel==\n";

	
    
