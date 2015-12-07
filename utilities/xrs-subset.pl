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

my $output;
my $rulef;

my @options=(
             q{given nbest input and xrs rule file, output subset of rules},
             q{note that sbtm-score.pl can more efficiently produce rule files from brf / grammar archives},
             ,["output=s"=>\$output,"rule subest output here"]
             ,["rulefile=s"=>\$rulef,"original rules read from here"]
             );

my $unk='@UNKNOWN@';
my ($usagep,@opts)=getoptions_usage(@options);
&show_cmdline;
show_opts(@opts);

&argvz;

&openz_stdout($output) if $output;
my $R=&openz($rulef);

my $capture_3brackets='{{{(.*?)}}}';
my $capture_natural='(\d+)';
my $capture_int='(-?\d+)';

my %ruleids;
while(<>) {
    if (/\bderivation=$capture_3brackets/o) {
        my $derivation=$1;
        $derivation=~s/\[\d+,\d+\]//g; # remove span marking if --show-span
        while ($derivation =~ /$capture_int/go) {
            my $id=$1;
            $ruleids{$id}=1;
            &count_info_gen("id=[$id]");
        }
    }
#    if (/\b\Q$hyp\E=$capture_3brackets/
}

while(<$R>) {
    if (/\bid=$capture_int/o) {
        my $id=$1;
        print if exists $ruleids{$id};
    }
}


flush();

&all_summary;
