#!/usr/bin/env perl

# input: discounted probabilities
# output: discounted probabilities with backoff weights

sub emit_line {
  my $p;
  my $bow = 1-$sum;
  for ($i=0; $i<=$#probs; $i++) {
    $x = $events[$i];
    $p = $probs[$i];
    print "$p\t$prevcontext $x\t$bow\n";
  }
}

while (<STDIN>) {
  chomp;
  ($p, @fields) = split;
  $order = $#fields+1;
  $context = join(" ", @fields[0..$#fields-1]);
  $event = $fields[$#fields];
  if (defined($prevcontext) && $context ne $prevcontext) {
    emit_line;
    @probs = ();
    @events = ();
    $sum = 0;
  }
  if (defined($prevevent) && $context eq $prevcontext && $event eq $prevevent) {
      $probs[$#probs] += $p;
  } else {
      push(@probs, $p);
      push(@events, $event);
  }

  $sum += $p;
  $prevcontext = $context;
  $prevevent = $event;
}

if (defined($prevcontext)) {
  emit_line;
}




