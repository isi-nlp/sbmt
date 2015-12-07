#!/usr/bin/env perl

#(graehl) create xrs rules from ATS "LEXICON" union alignments fractional counts

use strict;
use warnings;
use Getopt::Long;

use FindBin;
use lib $FindBin::RealBin;
use green;
### script info ##################################################
my $BLOBS;
BEGIN {
    $ENV{BLOBS}='/home/hpc-22/dmarcu/nlg/blobs' unless exists $ENV{BLOBS};
    $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
    $BLOBS=$ENV{BLOBS};
    my $libgraehl="$BLOBS/libgraehl/unstable";
    push @INC,$libgraehl if -d $libgraehl;
}

require "libgraehl.pl";
require "libxrs.pl";

### arguments ####################################################
my $BLOBS_TEMPLATE="{blobs}";
my $SCRIPT_TEMPLATE="{scriptdir}";

my $infile_name;
my $outfile_name;

my $baseid=-100000000;

my $evcb="e.vcb";
my $fvcb="f.vcb";
my $LEXICON="LEXICON";
my $lexind="lexflag";
my $lexef="lexprob";
my $lexfe="lexprobinv";
my $pruneby="unit_count_it1";
my $tagfile="{scriptdir}/LEXICON.BROWN.AND.WSJ";
my $cutoff=.05;
my $keepmin=5;
my $keepminpertag=1;
my $precision=4;
my $scale=1e-20;
my $logfile_name;
my $absdiscount=.5;
my $mincount=.1;

my $usevocab=0;

my @opts_usage=("creates xrs rules from ATS \"LEXICON\" union alignments fractional counts",
                ["outfile=s" => \$outfile_name,"Write output here (can be .gz)"],
                ["evcb=s" => \$evcb,"English vocab file"],
                ["fvcb=s" => \$fvcb,"Foreign vocab file"],
                ["usevocab!" => \$usevocab, "Normalize by count in e,f vocabs (includes unaligned instances!) when nonzero, instead of just the sum of fractional counts"],
                ["lexicon=s" => \$LEXICON,"LEXICON file (contains fractional counts of symmetrized alignments - lines are = <count> <fword> <eword>) (additional positional ARGV parameters are also used)"],
                ["tagfile=s" => \$tagfile,"Part of speech tags file from Brill tagger (first word = english word, second word = most common POS tag)"],
                ["xrs-baseid=i" => \$baseid, "If set, start rule id=N here, increasing in distance from 0"],
                ["indicator-field=s" => \$lexind, "Name for binary indicator feature (value=1)"],
                ["prob-field=s" => \$lexef, "Name for probability of e given f (aka 'normal') feature"],
                ["probinv-field=s" => \$lexfe, "Name for probability of f given e (aka 'inverse') feature"],
                ["prune-field=s" => \$pruneby, "Name for combined (geometric average) probability field (useful for top-k lexical pruning)"],
                ["prune-scale=s" => \$scale, "Multiply the prune-field value by this - useful to ensure that normal rules are always sorted as higher prob than lexicon rules in top-k lexical pruning"],
                ["keepmin-per-f=i" => \$keepmin, "Keep at least this many e given f, no matter how bad the probability"],
                ["keepmin-per-tag=i" => \$keepminpertag, "Keep at least this many e of each part of speech and f, no matter how bad the probability"],
                ["absolute-count-discount=s" => \$absdiscount, "Subtract this much from all fractional counts (minimum of $mincount)"],
                ["prob-cutoff=s" => \$cutoff, "Throw away (except for --keepmin) any rules with geometric avg prob and probin less than this"],
                ["precision=s" => \$precision,"Number of decimal places to keep (more gives bigger files without any useful discriminative information)"],
                ["logfile=s" => \$logfile_name,"(optional) write diagnostic messages here (as well as STDERR)"],
);

my $cmdline=&escaped_cmdline;
my ($usagep,@opts)=getoptions_usage(@opts_usage);

&outz_stderr($logfile_name);

info("COMMAND LINE:");
info($cmdline);
show_opts(@opts);

expand_opts(\@opts_usage,[$BLOBS_TEMPLATE,$BLOBS],[$SCRIPT_TEMPLATE,$FindBin::RealBin]);

set_default_precision($precision);

### main program ################################################
&set_ioenc("raw"); # or encoding(gb..something)? #use is lexicaly scoped so:
use open IN => ':raw';
use open OUT => ':raw';

my (%ecnt,%fcnt);
if ($usevocab) {
    info("Reading vcb files...");
    read_vocab_counts($evcb,\%ecnt);
    read_vocab_counts($fvcb,\%fcnt);
}

read_brill_pos_tags($tagfile) if $tagfile;

if ($LEXICON) {
    info("Reading lexicon from $LEXICON");
    unshift @ARGV,$LEXICON;
}
&argvz;

info("Reading LEXICON...");
my (%Lex,%ZF,%ZE);
while (<>) {
    chomp;
    my ($fraccount, $f, $e) = split;
    $fraccount-=$absdiscount;
    $fraccount=$mincount if $fraccount < $mincount;
    $ZF{$f} += $fraccount;
    $ZE{$e} += $fraccount;
#    next if $f eq 'UNKNOWN_WORD'; # good way to do unknown word smoothing i think
#    next if $e eq 'UNKNOWN_WORD'; # never seen in lexicon, only for F ...
    $Lex{$f}{$e} = $fraccount;
}

if ($usevocab) {
    for ([\%ZF,\%fcnt],[\%ZE,\%ecnt]) {
        my ($rz,$rv)=@$_;
        for (keys %$rz) {
            if (exists $rv->{$_}) {
                my $newc=$rv->{$_};
                my $oldc=$rz->{$_};
                if ($newc > 0) {
                    $rz->{$_}=$newc;
                    log_numbers("Replacing old total count(aligned)=$oldc with count=$newc from vocab");
                } else {
                    warning("0 vocab count for word $_");
                }
            } else {
                warning("missing vocab entry for word $_ - normalizing by sum of aligned words");
            }
        }
    }
}

outz_stdout($outfile_name);

my $id_inc=(defined $baseid && $baseid < 0) ? -1 : 1;

for my $f (sort keys %Lex) {
    my $nkept=0;
    my %nkeptpertag=();
    my @e_count;
    push_pairs_hash_to_list($Lex{$f},\@e_count);
    @e_count=sort { $b->[1] <=> $a->[1] } @e_count;
    for  (@e_count) {
        my ($e,$count)=@$_;
        my $probinv = $count/$ZE{$e};
        my $prob = $count/$ZF{$f};
        my $avgprob=sqrt($prob*$probinv);
        &debug("LEXICON:",$e,$f,$prob,$probinv,$avgprob);
#        log_numbers("prob=$prob probinv=$probinv avg=$avgprob");
        my $goodprob=($avgprob >= $cutoff);
        my $pos = getBrillTagDefault($e);
        my $keepforpos=$nkeptpertag{$pos}++ < $keepminpertag;
        my $keepforf=$nkept++ < $keepmin;
        unless ($keepforpos || $keepforf || $goodprob) {
#            log_numbers("skipping rule with avgprob=$prob less than $cutoff");
            info_remember("skipping rule with low avgprob"," of $avgprob ($e -> $f)");
            next;
        }
        if ($goodprob) {
            info_remember_quiet("kept rule because its prob exceeded $cutoff");
        } elsif ($keepforf) {
            info_remember_quiet("kept low prob rule because we wanted at least $keepmin E for every F");
        } else {
            info_remember_quiet("kept low prob rule because we wanted at least $keepminpertag E for every (tag(E),F)");
        }
        print xrs_lexical_rule($pos,$e,$f);
        if ($baseid) {
            print " id=$baseid";
            $baseid+=$id_inc;
        }
        print " $lexind=1 $lexef=".real_to_ehat($prob)." $lexfe=".real_to_ehat($probinv);
        print " $pruneby=".real_to_ehat($avgprob*$scale) if ($pruneby);
        print "\n";
    }
}


if ($infile_name) {
    info("Adding infile = $infile_name");
    unshift @ARGV,$infile_name;
}


&info_summary;
&print_number_summary;
#&restore_stdout if $outfile_name;
#&restore_stderr if $logfile_name;
