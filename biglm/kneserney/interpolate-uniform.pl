#!/usr/bin/env perl

# input: discounted probabilities
# output: probabilities interpolated with uniform distribution

while (<STDIN>) {
  chomp;
  ($event, $p) = split(/\t/);
  if (defined($prevevent) && $event eq $prevevent) {
      $probs[$#probs] += $p;
  } else {
      push(@probs, $p);
      push(@events, $event);
  }

  $sum += $p;
  $prevevent = $event;
}

my $bow = 1-$sum;
for ($i=0; $i<=$#probs; $i++) {
    $x = $events[$i];
    $p = $probs[$i];
    $p += $bow*1./@probs;
    print "$x\t$p\n";
}



