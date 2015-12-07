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
my $BLOBS;

BEGIN {
   $scriptdir = &File::Basename::dirname($0);
   ($scriptname) = &File::Basename::fileparse($0);
    $ENV{BLOBS}='/home/hpc-22/dmarcu/nlg/blobs' unless exists $ENV{BLOBS};
   $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
    $BLOBS=$ENV{BLOBS};
    my $libgraehl="$BLOBS/libgraehl/unstable";
    unshift @INC,$libgraehl if -d $libgraehl;
    my $decoder="$BLOBS/mini_decoder/unstable";
    unshift @INC,$decoder if -d $decoder;
    unshift @INC, $scriptdir;
}

require "libgraehl.pl";

### arguments ####################################################

my $output='-';

my @options=(
q{read (lisp format) english parse trees with (NT~2~4 => same as (NT, and write grammar.nts format NT counts file},
["output=s"=>\$output,"write (grammar.nts format) NT count(NT) lines here"],
);

&show_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
show_opts(@opts);

&argvz;

&outz_stdout($output);

my %c;
while(<>) {
    while (
           #m{\(([^ ~]+)}g
           m{(?: |^)\(([^() ~]+)(?: |~\d+~\d+ )}g
          )
      {
          inc_hash(\%c,$1);
      }
}

my @sk=sort { $c{$b} <=> $c{$a} } keys %c;
print "$_ $c{$_}\n" for (@sk);
