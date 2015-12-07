#!/usr/bin/env perl
#
# Author: graehl

use strict;
use warnings;
use Getopt::Long;

### script info ##################################################
use File::Basename;
my $scriptdir; # location of script
my $scriptname; # filename of script

BEGIN {
   $scriptdir = &File::Basename::dirname($0);
   ($scriptname) = &File::Basename::fileparse($0);
    unshift @INC, $scriptdir;
}

require "libgraehl.pl";

### arguments ####################################################

my $seed=3289023804;
my $min=.2;
my $max=.8;
my $initparam;
my $id_origin=2;
my $out;

my @options=(
qq{STDIN = tiburon rules.  output = tiburon rules w/ random weights between $max and $min.  also write initial weights file with id_origin for forest-em},
["initparam=s"=>\$initparam,"write weights for forest-em here.  note: first value is for id=1"],
["out=s"=>\$out,"write tiburon rules w/ weights here"],
["min=s"=>\$min,"minimum random param"],
["max=s"=>\$max,"maximum random param"],
["xrs-id-origin=s"=>\$id_origin,"start counting at id=N for the top->start rule"],
);

&show_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
show_opts(@opts);

&argvz;

&outz_stdout($out) if $out;

my $I=openz_out($initparam) if $initparam;

my $start=<>;
die unless $start;
print $start;


srand($seed);

my $d=$max-$min;


sub rand_weight {
    rand($d)+$min;
}

my $FP=&number_re;
my $ID=&integer_re;

if ($I) {
    print $I "(\n";
    print $I " 1\n" for (1..$id_origin);
}

#my $n=0;
while(<>) {
#    ++$n;
    my $w=&rand_weight;
    if ($I) {
        print $I " $w\n";
    }
    if (s/\s*(?:#\s+($FP))?\s*(?:\@\s+($ID))?$//) {
        my $weight=$1;
        my $tiegroup=$2;
        die "can't assign random parameter: tied tiburon rule" if defined $tiegroup;
        chomp;
        print "$_ # $w\n";
    } else {
        die "not a tiburon rule: $_";
    }
}

if ($I) {
    print $I ")\n";
}

&info_summary;
