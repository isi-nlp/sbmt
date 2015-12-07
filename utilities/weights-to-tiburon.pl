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

my $output='-';
my $weightname='emprob';
my $weights;
my $humanp;
my $zerobelow=0;
my $removezeros;

my @options=(
qq{read Tiburon format tree->string transducer rules, and corresponding line by line id=N $weightname=X lines, writing Tiburon rules with weight X. Note: first line of tiburon rules is an initial state, which has a corresponding weight, but that weight is not printed in the output.},
["output=s"=>\$output,"write tiburon rules here"],
["weight-lines=s"=>\$weights,"lines corresponding to input, with '$weightname=X' meaning the weight is X"],
["fieldname=s",\$weightname,"attribute name for tiburon rule weight"],
["human-probs!",\$humanp,"don't output 'e^0', output '1' (e^-300 rounds to 0)"],
["zero-below=s",\$zerobelow,"zero any probs below this"],
["remove-zeros!",\$removezeros,"omit any rules with 0 prob"],
);

&show_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
show_opts(@opts);

&argvz;

&outz_stdout($output);

my $W=openz($weights);

my $FP=&number_re;
my $ID=&integer_re;

sub getwt
{
    local $_;
    while (<$W>) {
        return ($humanp ? getreal($1) : $1) if /\b\Q$weightname\E=(\S*)/;
    }
};

my $skip=1;

$zerobelow=getreal($zerobelow);

while (<>) {
# it's \s*(#\s+weight)?\s*(@\s+id)$ where weight is some double and id is some
# int, but yes. in case i got the regexp wrong, I mean after the rule "optional
# whitespace then, if a weight, # followed by some space followed by the weight,
# then, if a tie, @ followed by some space followed by the tie. both are
# optional; if both are present the weight comes first.
    superchomp(\$_);
    my $w=&getwt;
    $w=0 if getreal($w) < $zerobelow;
    next if $w==0 && $removezeros;
    next unless $_;
    die "no weight for line #$." unless defined $w;
#    &debug($_);
    s/\s*(?:#\s+$FP)?(\s*\@\s+($ID))?$//;
    my $tie=$1;
    $tie=defined $tie ? " $tie" : '';
    chomp $tie;
    if ($skip > 0) {
        --$skip;
        print "$_\n";
    } else {
        print "$_ # $w$tie\n";
    }
}
