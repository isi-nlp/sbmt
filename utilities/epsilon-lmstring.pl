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

my ($line1,$idempotent)=(1,1);
my ($output,$translated,$best_only,$nbest_only);

my %name=qw(
            lmstring lm_string
           );
my $unkstr='@UNKNOWN@';

my @options=(
q(replaces @UNKNOWN@ in lmstring={{{"@UNKNOWN"}}} - good idea if you're rebinarizing xrs rules and want the same behavior as unknown_word_rules),
,["output=s"=>\$output,"send output here (instead of stdout)"]
,["unknown-word=s"=>\$unkstr,"look for this word in lmstring (default same as unknown_word_rules)"]
);


&show_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
show_opts(@opts);

&argvz;
&outz_stdout($output);
my $enc='utf8';
&set_ioenc($enc);

my $capture_3brackets=&capture_3brackets_re;

while(<>) {
    my $line=$_;
    if ($line=~/$name{lmstring}=$capture_3brackets/) {
        my ($a,$b)=($-[1],$+[1]);
        my $l=$b-$a;
        my $lms=substr($line,$a,$l);
        my $lmpost=$lms;
        count_info("found lmstring");
        $lmpost =~ s/"\Q$unkstr\E"//g;
        if ($lmpost ne $lms) {
            $lmpost =~ s/  +/ /g;
            info_remember_gen("changed lmstring [$lms] => [$lmpost]");
            substr($line,$a,$l,$lmpost)
        }
    }
    print $line;
}

&info_summary;
