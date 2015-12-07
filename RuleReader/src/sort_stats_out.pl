#! /usr/bin/perl

use strict;

if( scalar(@ARGV) != 1 )
{
  &print_usage();
  exit 1;
}

open IN, $ARGV[0] or die "Couldn't open file: $ARGV[0]";

my %hash;

while( <IN> )
{
  chomp;

  /(\S+) (\S+) (\{.*\})/;

  # hash the label to a pair containing the count and the list of rules
  $hash{$1} = [$2, $3];
}

close IN;

# sort them and print them out
my @sorted = sort {$hash{$b}->[0] <=> $hash{$a}->[0]} keys(%hash);

for(@sorted)
{
  print "$_: " . $hash{$_}->[0] . ": " . $hash{$_}->[1] . "\n";
}


sub print_usage
{
  print STDERR "sort_stats_out.pl <file>\n";
  print STDERR "  <file> contains one label per line as \"<label> count\"\n";
}
