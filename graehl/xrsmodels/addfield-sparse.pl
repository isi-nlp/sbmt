#!/usr/bin/env perl

#(graehl) append attributes to xrs rules, from a sparse vector (intended for SGT
#freq of freq)

use strict;
use warnings;
use Getopt::Long;
use File::Basename;

my $blobbase="/home/hpc-22/dmarcu/nlg/blobs";

### script info ##################################################
my $scriptdir;                  # location of script
my $scriptname;                 # filename of script
my $BLOBS;

BEGIN {
    $scriptdir = &File::Basename::dirname($0);
    ($scriptname) = &File::Basename::fileparse($0);
    push @INC, $scriptdir;      # now you can say: require "blah.pl" from the current directory
    $ENV{BLOBS}='/home/hpc-22/dmarcu/nlg/blobs' unless exists $ENV{BLOBS};
    $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
    $BLOBS=$ENV{BLOBS};
    my $libgraehl="$BLOBS/libgraehl/unstable";
    push @INC,$libgraehl if -d $libgraehl;
}

require "libgraehl.pl";

### arguments ####################################################
my $BLOBS_TEMPLATE="{blobs}";

my $infile_name;
my $outfile_name;

my $sparse_name;
my $keyfield="sent_count";
my $valfield="sent_sgt_ratio";
my $prob=1;

my $precision=4;
my $overwrite=1;

my @opts_usage=("append attributes to xrs rules, from a sparse vector (intended for SGT freq of freq)",
                ["infile=s" => \$infile_name,"Take <file> as input (as well as the rest of ARGV)"],
                ["outfile=s" => \$outfile_name,"Write output here (can be .gz)"],
                ["sparsevectorfile=s" => \$sparse_name,"Read sparse 'key value' lines from here"],
                ["keyfieldname=s"=>\$keyfield,"look for keyfield=s and insert sparsevectorfile{s} as valfield=value (if found)"],
                ["valfieldname=s"=>\$valfield,"write valfield=value"],
                ["removeoldval!"=>\$overwrite,"remove old valfield=oldvalue (unconditionally!)"],
                ["prob!"=>\$prob,"If set, always output e^N such that e^N=value (assumes values >0)"],
                ["precision=s" => \$precision,"Number of decimal places to keep (more gives bigger files without any useful discriminative information)"],
                
);

my $cmdline=&escaped_cmdline;
my ($usagep,@opts)=getoptions_usage(@opts_usage);

info("COMMAND LINE:");
info($cmdline);
show_opts(@opts);

expand_opts(\@opts_usage,[$BLOBS_TEMPLATE,$BLOBS]);

set_default_precision($precision);

### main program ################################################

if ($infile_name) {
    info("Adding infile = $infile_name");
    unshift @ARGV,$infile_name;
}
outz_stdout($outfile_name);

my %sparse;
open SPARSE,'<',$sparse_name or die "no $sparse_name - $!";
while(<SPARSE>) {
    /^\s*(\S+)\s+(\S+)\s*$/ or die "error - expected space separated key val pairs in --sparsevectorfile $sparse_name";
    $sparse{$1}=$2;
}

&argvz;
while(<>) {
    my $line=$_;
    if ($overwrite && $line=~s/ ?(\Q$valfield\E)=({{{(.*?)}}}|([^{]\S*))//g) {
#        &debug("removed",$valfield,$2);
        info_remember_quiet("removed old $valfield");
#        log_numbers("removed $valfield=$2");
    }
    if ($line=~/(\Q$keyfield\E)=({{{(.*?)}}}|([^{]\S*))/) {
        my $braces=1 if defined $3;
        my $key=($braces ? $3 : $4);
        my $val;
        info_remember_quiet("$keyfield found for line");
        $val = $sparse{$key} if exists $sparse{$key};
        if (defined $val) {
            info_remember_quiet("$keyfield had a key; attached $valfield","; $key=$val");
            chomp $line;
            $val = real_to_ehat($val) if $prob;
#            log_numbers("added for $keyfield=$key: $valfield=$val");
            print "$line $valfield=$val\n";
            next;
        }
    } else {
        info_remember_quiet("No $keyfield found for line");
    }
    print $line;
}

&info_summary;
#&print_number_summary;
