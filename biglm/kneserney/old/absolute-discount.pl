#!/usr/bin/env perl

# input: counts
# output: discounted probabilities, with backoff weights

$d_max = 3;

open COUNTS, $ARGV[0] || die;
while (<COUNTS>) {
  chomp;
  ($cc, $c) = split(" ",$_,2);
  $countcount{$c} = $cc if ($c <= $d_max+1);
}
close COUNTS;

#@mincount = (0,1,1,2,2,2,2,2,2); # this is the SRI-LM default
@mincount = (0,1,1,1,1,2,2,2,2);

$d[0] = 0;

# Original Kneser-Ney
#$d[1] = $countcount{1}/($countcount{1}+2*$countcount{2});

# Modified Kneser-Ney
$y = $countcount{1}/($countcount{1}+2*$countcount{2});
for ($i=1; $i<=$d_max; $i++) {
    $d[$i] = $i-($i+1)*$y*$countcount{$i+1}/$countcount{$i};
}

print STDERR "Kneser-Ney: D = ", join(" ", @d), "\n";

sub emit_line {
  my $p;
  my $bow = 0;
  for ($i=0; $i<=$#probs; $i++) {
    $c = $probs[$i];
    $c_ceil = $c > $#d ? $#d : $c;
    if ($c < $mincount[$order]) {
      $probs[$i] = 0;
      $bow += $c/$sum;
    } else {
      $probs[$i] = ($c-$d[$c_ceil])/$sum;
      $bow += $d[$c_ceil]/$sum;
    }
  }
  
  for ($i=0; $i<=$#probs; $i++) {
    $x = $events[$i];
    $p = $probs[$i];
    #print "$p\t$prev $x\t$bow\n";
    print "$p\t$prev $x\n";
  }
}

while (<STDIN>) {
  chomp;
  ($count, @fields) = split;
  $order = $#fields+1;
  $context = join(" ", @fields[0..$#fields-1]);
  $event = $fields[$#fields];
  if (defined($prev) && $prev ne $context) {
    emit_line;
    @probs = ();
    @events = ();
    $sum = 0;
    $n1 = 0;
  }
  $sum += $count;
  $n1 += 1;
  $prev = $context;
  push(@probs, $count);
  push(@events, $event);
}

if (defined($prev)) {
  emit_line;
}




