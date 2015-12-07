#!/usr/bin/env perl

open PROBFILE, $ARGV[0] || die;
open BACKOFFFILE, $ARGV[1] || die;

# Expected format of files is
# prob word+ bow

# Lower-order file is expected to be sorted
# Higher-order file is expected to be sorted starting from the 2nd word
# i.e., they should have the same order

sub find_backoff {
    my $goal = $_[0];
    while ($boline = <BACKOFFFILE>) {
        chomp $boline;
	my @fields = split(" ", $boline);
	my $backoffcheck = join(" ", @fields[1..$#fields-1]);
	my $backoffprob = $fields[0];
	if ($backoffprob =~ /^\*(.*)$/) { 
	  $backoffprob = $1;
	}
	if ($backoffcheck eq $goal) {
	    return $backoffprob;
	}
    }
    die "files not aligned (EOF on $ARGV[1] looking for $goal)";
}

while (<PROBFILE>) {
    $empty = 0;
    chomp;
    @fields = split;
    $prob = $fields[0];
    $word1 = $fields[1];
    $backoff = join(" ", @fields[2..$#fields-1]);
    $bow = $fields[$#fields];
    if (!defined($prev) || $prev ne $backoff) {
	$backoffprob = find_backoff($backoff);
    }
    if ($prob == 0) {
      # this star means that this prob can be safely removed
      $prob = "*" . $bow*$backoffprob;
    } else {
      $prob += $bow*$backoffprob;
    }
    print "$prob\t$word1 $backoff\t$bow\n";

    $prev = $backoff;
}

die "files not aligned (EOF on $ARGV[0])" if (defined(<BACKOFFFILE>));
