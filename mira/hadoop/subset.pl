#!/usr/bin/perl

if ($ARGV[0] eq "-r") {
    $random = 1;
    shift;
}

open SUBSETFILE, $ARGV[0] || die "couldn't open sub-info file $ARGV[0]";
open KEYFILE, $ARGV[1] || die "couldn't open info file $ARGV[1]";;
open VALFILE, $ARGV[2] || die "couldn't open data file $ARGV[2]";;

if ($random) {
    # allows the info file to be in any order
    @subset = ();
    while (<SUBSETFILE>) {
	$seek{$_} = 1;
	push @subset, $_;
    }
    while ($keyline = <KEYFILE>) {
	$valline = <VALFILE>;
	if (!defined($valline)) {
	    die "warning: $ARGV[2] is shorter than $ARGV[1]\n";
	}
	if (exists $seek{$keyline}) {
	    if (exists $value{$keyline} && $valline ne $value{$keyline}) {
		print STDERR "warning: duplicate lines in $ARGV[1] with different values in $ARGV[2]:\n";
		print STDERR "  duplicate info lines: $keyline\n";
		print STDERR "  conflicting value lines:\n";
		print STDERR "    $value{$keyline}\n";
		print STDERR "    $valline\n";
	    } else {
		$value{$keyline} = $valline;
	    }
	}
    }
    if (<VALFILE>) {
	die "warning: $ARGV[1] is shorter than $ARGV[2]\n";
    }
    foreach $line (@subset) {
	if (exists $value{$line}) {
	    print $value{$line};
	} else {
	    die "couldn't find key $subsetline in $ARGV[1] requested in $ARGV[0]";
	}
    }
} else {
    while ($subsetline = <SUBSETFILE>) {
	$found = 0;
	while ($keyline = <KEYFILE>) {
	    $valline = <VALFILE>;
	    if (!defined($valline)) {
		die "warning: $ARGV[2] is shorter than $ARGV[1]\n";
	    }
	    if ($keyline eq $subsetline) {
		print $valline;
		$found = 1;
		last;
	    }
	}
	if (!$found) {
	    die "couldn't find key $subsetline in $ARGV[1] requested in $ARGV[0]";
	}
    }

    while (<KEYFILE>) {
	$valline = <VALFILE>;
	if (!defined($valline)) {
	    die "warning: $ARGV[2] is shorter than $ARGV[1]\n";
	}
    }
    
    if (<VALFILE>) {
	die  "warning: $ARGV[1] is shorter than $ARGV[2]\n";
    }
}
