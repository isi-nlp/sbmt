#!/usr/bin/perl -w

# genpoints.pl
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.


if (@ARGV != 2) {
    die "usage: genpoints.pl <ranges> <num-points>"
}
$s = $ARGV[0];
$n = $ARGV[1];

@lo = ();
@hi = ();

foreach $x (split(" ", $s)) {
  if ($x =~ /(-?[\d.]+)-(-?[\d.]+)/) {
    push(@lo, $1);
    push(@hi, $2);
    $d++;
  } else {
    print STDERR "bad weight range: $x\n";
  }
}

for ($i=0; $i<$n; $i++) {
    for ($j=0; $j<$d; $j++) {
	if ($hi[$j]-$lo[$j] == 0) {
	    print " ", $lo[$j]; # because rand(0) is the same as rand(1) -- stupid
        } else {
	    print " ", rand($hi[$j]-$lo[$j])+$lo[$j];
	}
    }
    print "\n";
}
