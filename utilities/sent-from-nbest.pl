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
    $ENV{BLOBS}='/auto/nlg-01/blobs' unless exists $ENV{BLOBS};
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
my $n=1;

my %name=qw(
            hyp hyp
            NBEST NBEST
            sent sent
            nbest nbest
           );

my @options=(
             q{given nbests on input, pull out one sent=N and call it sent=1},
             ,["output=s"=>\$output,"modified LM output here"]
             ,["n=i"=>\$n,"pull out sent=n"]
             );

my $unk='@UNKNOWN@';
my ($usagep,@opts)=getoptions_usage(@options);
&show_cmdline;
show_opts(@opts);

&argvz;

&openz_stdout($output) if $output;

my $capture_3brackets=&capture_3brackets_re;
my $capture_natural='(\d+)';

while(<>) {
    if (/^\Q$name{NBEST}\E\s+\Q$name{sent}\E=$capture_natural\s+/o) {
        my $sent=$1;
        if ($sent == $n) {
            s/(^\Q$name{NBEST}\E\s+\Q$name{sent}\E)=$capture_natural/$1=1/;
            print;
        }
    }
}

flush();

#&all_summary;
