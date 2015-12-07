#!/usr/bin/perl -w
#
#  extract-at.pl usec < logfile
#
#  Go over argmin output and extract the 1-best output after usec user
#  seconds have elapsed (per sentence)
#
#  $Id: extract-at.pl 1303 2006-10-05 05:32:51Z jturian $
#

die $! unless scalar @ARGV == 1;
$usec = $ARGV[0];

@best = ();
while(<STDIN>) {
	if (m/\(intermediate\) \[\S+ wall sec, (\S+) user sec, \S+ system sec, \S+ major page faults\] (NBEST.*)/) {
		next if $1 > $usec;
		$out = $2;
		if (m/sent=([0-9]+)/) {
			$best[$1] = $out;
		} else {
			die $!;
		}
	}
}

foreach $k (@best) {
	next if not $k;
	print $k, "\n";
}
