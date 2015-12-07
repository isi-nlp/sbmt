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
q{read decoder xrs rules, write joint normalization groups (by id=N) for forest-em},
["out-normgroups=s"=>\$output,"write forest-em normalization groups here (joint, i.e. state, normalization)"],
);

&show_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
show_opts(@opts);

&argvz;

&outz_stdout($output);

my %ng;
while (<>) {
    if (/^([^(]+)\(.* ### .*id=(-?\d+)\b/) {
        my ($nt,$id)=($1,$2);
        push @{$ng{$nt}},$id;
    }
}

print "(\n";
while (my ($nt,$ids)=each %ng) {
    print " (";
    print ' ',$_ for (@$ids);
    print " )\n";
}
print ")\n";
