#!/usr/bin/env perl

# Somewhere around CMPH version 0.8, the file format changed just slightly.
# To prevent incompatibility, DC modified CMPH to stick with the old format
# (it's just a float vs. double). This script converts biglm noisy LM files
# from the new format to the old, compatible format.

open(IN, $ARGV[0]) || die;
open(OUT, ">$ARGV[1]") || die;
binmode(IN);
binmode(OUT);

$count = 0;
$buf = "";
$find = "brz\0";
while (read(IN, $c, 1) > 0) {
    print OUT $c;
    $buf .= $c;
    $buf = substr($buf, -length($find));
    if ($buf eq $find) {
	print STDERR "found: $find\n";

	read(IN, $m, 4);
	print STDERR "m=", unpack('L', $m), "\n";
	print OUT $m;
	
        # convert double to float
	read(IN, $c, 8);
	print STDERR "c=", unpack('d', $c), "\n";

	$f = unpack('d', $c);
	if (2.6 <= $f && $f <= 3.0) { # these are sane values for this parameter
	    print STDERR "repairing\n";
	    $c = unpack('d', $c);
	    $c = pack('f', $c);
	    $count++;
	}
	print OUT $c;
    }
}

print STDERR "$count occurrences repaired\n";
