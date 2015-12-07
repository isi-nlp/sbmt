#!/usr/bin/env perl

# Relocates the BOWs from the (n+1)-gram file into the n-gram file.
# They don't logically belong here but they are more compact to store
# this way.

# Expected format of files is
# prob word+ bow

# Both files are expected to be sorted

# Also, we change probabilities into log-probabilities here.

open LOWER, $ARGV[0] || die;
if ($#ARGV >= 1) {
    open HIGHER, $ARGV[1] || die;
} else {
    $nobows = 1;
}

sub find_bow {
  return undef if ($nobows);
  my $goal = $_[0];
  my $words;
  my $bow;
  while (1) {
    if (defined $save_words) {
      $words = $save_words;
      $bow = $save_bow;
      undef $save_words;
    } elsif (defined ($hline = <HIGHER>)) {
      chomp $hline;
      my @fields = split(" ", $hline);
      $words = join(" ", @fields[1..$#fields-2]);
      $bow = $fields[$#fields];

      next if (defined $prevwords and $words eq $prevwords);
      $prevwords = $words;
    } else {
      undef $bow;
      last;
    }

    if ($words eq $goal) {
      # we found it
      last;
    } elsif ($words gt $goal) {
      # we passed it -- save this context for next time
      $save_words = $words;
      $save_bow = $bow;
      $bow = undef;
      last;
    } else {
      # we found a context not in lower-order file
      # insert it into the output with a dummy prob
      print_line(undef, $words, $bow);
    }
  }
  return $bow;
}

$special = qr{<\/s>$|^<unk>$}; # we don't expect these to have BOWs

sub print_line {
  my ($prob, $words, $bow) = @_;
  my $weak;
  if (defined $prob) {
    if ($prob =~ /^\*(.*)$/) { 
	$weak = 1;
	$prob = log($1)/log(10);
    } else {
	$weak = 0;
      $prob = log($prob)/log(10);
    }
  } else {
    # this should really only happen for <s>
    print STDERR "creating dummy entry for $words\n";
    $prob = -99;
  }
  if (defined $bow) {
    $bow = log($bow)/log(10);
    if ($weak) {
	printf "*%.7g\t%s\t%.7g\n", $prob, $words, $bow;
    } else {
	printf "%.7g\t%s\t%.7g\n", $prob, $words, $bow;
    }
  } else {
      if ($weak) {
	  printf "*%.7g\t%s\n", $prob, $words;
      } else {
	  printf "%.7g\t%s\n", $prob, $words;
      }
  }
}

while (<LOWER>) {
    chomp;
    @fields = split;
    $prob = $fields[0];
    $words = join(" ", @fields[1..$#fields-1]);
    $bow = find_bow($words);
    if (!defined $bow && !$nobows && $words !~ $special) {
      die "files not aligned (looking for $words)";
    }
    print_line($prob, $words, $bow);
}

#die "files not aligned (EOF on $ARGV[0])" if (defined(<HIGHER>));
