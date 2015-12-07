#!/usr/bin/env perl


# Expected format of files is
# prob word+ bow

# Both files are expected to be sorted

$order = $ARGV[0];

open LOWER, $ARGV[1] || die;
if ($#ARGV >= 2) {
    open HIGHER, $ARGV[2] || die;
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
      $words = join(" ", @fields[1..$order]); # can't rely on $#fields because bows might not be present
      $bow = 1;

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
    }
  }
  return $bow;
}

while (<LOWER>) {
    chomp;
    @fields = split;
    $prob = $fields[0];
    $words = join(" ", @fields[1..$order]);
    $bow = find_bow($words);

    # a starred entry can be deleted if it's not a context for another event
    if ($prob =~ /^\*(.*)$/) {
      $prob = $1;
      next if (!defined $bow && $words !~ /^<unk>$/);
    }

    print "$prob\t", join(" ", @fields[1..$order]);
    print "\t$fields[$#fields]" if ($#fields == $order+1);
    print "\n";
}

