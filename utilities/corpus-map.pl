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
require "lib-corpus-map.pl";

### arguments ####################################################


my %name=qw(
            sent sent
           );

my $is_opl;
my $is_sent;
my $chunk_id;
my $output;
my $to_orig=1;
my $chunk_filename;
my $corpus_map;

my @options=(
             q(maps to/from Jens' corpus-map file sorted and original sentence indices),
             ,["corpus-map=s"=>\$corpus_map,"Use corpus-map file (Jens format) for remapping sentence numbers (both for sent=N and one-per-line line numbers"]
             ,["chunk-id=i"=>\$chunk_id,"Assume the input comes from this chunk (for sorted->original). otherwise lines are assumed to be combined (not chunked)."]
             ,["zero-indexed-sent!"=>\$zero_sent,"with sent=N, the first sentence starts at N=0"]
             ,["sent!"=>\$is_sent,"Transduce input with sent=N lines replaced with sent=F(N)"]
             ,["opl!"=>\$is_opl,"Input is one-per-line"]
             ,["to-orig"=>\$to_orig,"Output original corpus line number"]
             ,["chunk-filenames!"=>\$chunk_filename,"The first 2+ digit number in the input filenames sets the chunk-id"]
             ,["output=s"=>\$output,"Output here (default STDOUT)"]
);


&show_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
show_opts(@opts);

load_corpus_map($corpus_map);
&argvz;
&outz_stdout($output);
my $enc='utf8';
&set_ioenc($enc);

sub input_chunk {
    ($chunk_filename && $ARGV =~ /(\d\d+)/) ? $1 : $chunk_id;
}


my $chunk=$chunk_id;
my $nl;
sub lc2o {
    my $c=$chunk?" chunk $chunk":'';
    my $o=line_chunk_to_original($nl,$chunk);
    log_numbers("$c line $nl -> original line $o");
    return $o;
}

my @outlines;
my $capture_3brackets=&capture_3brackets_re;
my $capture_natural='\b(\d+)\b';
if ($to_orig) {
    my $lastV='';
    $nl=0;
    while (<>) {
        my $l=$_;
        if ($chunk_filename && $ARGV ne $lastV) {
            $nl=0;
            $chunk=&input_chunk;
        }
        count_info_gen("line from chunk $chunk") if ($chunk);
        if ($is_opl) {
            $outlines[&lc2o]=$l;
        }
        if ($is_sent) {
            if ($l =~ /\b\Q$name{sent}\E=$capture_natural/o) {
                $nl=$1;
                my $s=&lc2o;
#                log_numbers("sent=$nl -> sent=$s");
                print replace_span($l,$-[1],$+[1],$s)
            } else {
                print $l;
            }
        }
        ++$nl;
    }
} else {
    while (<>) {
        if ($is_opl) {
            my $s=original_to_sorted($.);
            log_nubmers("original line $. -> sorted line $s");
            $outlines[$s]=$_;
        }
    }
}

print @outlines if $is_opl;

&all_summary;
