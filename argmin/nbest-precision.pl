#!/usr/bin/perl -w
#
#	nbest-precision.pl nbest1 nbest2
#
#  For each sentence:
#	Determine what percent of candidates in nbest1 are also in nbest2 for this sentence.
#
#

die $! unless scalar @ARGV == 2;
$f1 = shift @ARGV;
$f2 = shift @ARGV;

%nbest = ();
%sentences = ();

# First, read in nbest2
open(F, "<$f2") or die $!;
while(<F>) {
	if (/^NBEST sent=([0-9]+) nbest=([0-9]+) (.*)/) {
		$sentences{$1} = 1;
		$nbest{$1}{$3} = 1;
#		print "$3\n" if $1 eq "1";
	}
}
close F;

%correct = ();
%candidates = ();

# Now, read in nbest1
open(F, "<$f1") or die $!;
while(<F>) {
	if (not /failed-parse=1/ and /^NBEST sent=([0-9]+) nbest=([0-9]+) (.*)/) {
		$sentences{$1} = 1;
		$candidates{$1}++;
		$correct{$1}++ if $nbest{$1}{$3};
#		print "$3\n" if $1 eq "1";
#		print "$3\n" if not $nbest{$1}{$3};
	}
}
close F;

@sent = sort keys %sentences;
foreach $s (@sent) {
	$correct{$s} = 0 if not $correct{$s};
	if (not $candidates{$s}) {
		print "$s NO CANDIDATES IN $f1\n";
	} else {
		print $s, " ", 100. * $correct{$s} / $candidates{$s}, "\n";
	}
}
