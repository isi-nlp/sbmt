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
            hyp hyp
            NBEST NBEST
            sent sent
            nbest nbest
           );
my $idempotent_feat="pasted-byline";
my $byline_length_feat="pasted-byline-length";
my @options=(
q(Filters copying input to output, restoring bylines to hyp of input nbests like: NBEST sent=1 nbest=3 hyp={{{jim died .}}} with modified hyp={{{[line #sent of translated-file ] jim died .}}}),
,["translated-from=s"=>\$translated,"expect bylines here"]
,["line-one-is-sent=i"=>\$line1,"sent=I for the first line of translated-from"]
,["output=s"=>\$output,"send output here (instead of stdout)"]
,["nbest-only!"=>\$nbest_only,"just output NBEST lines"]
,["best-only!"=>\$best_only,"just output NBEST ... nbest=0 lines (in fact, just the first such per sentence)"]
,["idempotent!"=>\$idempotent,"add $idempotent_feat={{{<byline>}}} $byline_length_feat=<#words(byline)>  (idempotently) for each nonempty translated-from byline added"]
);


&show_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
show_opts(@opts);

&argvz;
&outz_stdout($output);
my $enc='utf8';
&set_ioenc($enc);
my $T=&openz($translated);
binmode $T,$enc;

my %best_seen_sents;

my @trans;
my $n=$line1;
while(<$T>) {
    my $t=$_;
    superchomp(\$t);
    my $len=length $t;
    $t .= ' ' if $len > 0;
    $trans[$n++]=$t;
    &debug($t);
    count_info_gen("read [$len character] pre-translation from $translated)");
}

my $capture_3brackets=&capture_3brackets_re;
my $capture_natural='(\d+)';

while(<>) {
    if (/^\Q$name{NBEST}\E\s+\Q$name{sent}\E=$capture_natural\s+\Q$name{nbest}\E=$capture_natural/o) {
        my ($sent,$nbest)=($1,$2);
        my $do=!$best_only;
        if ($nbest eq '0') {
            my $bestname="$name{sent}=[$sent] $name{nbest}=0";
            if (inc_hash(\%best_seen_sents,$sent)) {
                warning_gen("duplicate $bestname");
            } else {
                $do=1;
                count_info_gen("got initial $bestname");
            }
        }
        if ($do) {
            if ($trans[$sent]) {
                my $byline=$trans[$sent];
                $byline =~ s/\s+$//;
                my $len=count_words($byline);
                count_info_gen("found a non-trivial [$len word] byline translation for sentence [$sent]");
                my $already_pasted= / \Q$idempotent_feat\E=$capture_3brackets/;
                my $old_byline=$1;
                warning("Found $idempotent_feat=1 already but since --idempotent not enabled, pasting byline anyway") 
                  if (!$idempotent && $already_pasted);
                $already_pasted=0 unless $idempotent;
                if ($already_pasted) {
                    count_info_gen("skipped a non-trival byline translation for sentence [$sent]");
                } else {
                    count_info_gen("pasted a non-trival byline translation for sentence [$sent]");
                    s/\b\Q$name{hyp}\E=$capture_3brackets/$name{hyp}={{{$byline $1}}}/;
                    s/$/ $idempotent_feat={{{$byline}}} $byline_length_feat=$len/ if $idempotent;
                }
            }
            print;
        }
        count_info_gen("read derivation for sentence [$sent]");
    } else {
        print unless ($nbest_only || $best_only);
        count_info("copied a non-nbest line");
    }
}

&info_summary;
