#!/usr/bin/env perl

#(graehl) "creates rules from manlex.e,manlex.f dictionaries"

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

my $baseid=-200000000;

my $evcb="e.vcb";
my $fvcb="f.vcb";
my $manlexpre="neverseen";
my $MANLEXPRE_TEMPLATE="{manlexpre}";
my $emanlex="$MANLEXPRE_TEMPLATE.manlex.e";
my $fmanlex="$MANLEXPRE_TEMPLATE.manlex.f";
my $indicator="unseen_manlex";
my $pruneby="unit_count_it1";
my $tagfile="{scriptdir}/LEXICON.BROWN.AND.WSJ";
my $pruneval="e^-66";
my $logfile_name;
my $tagdef='NNP';
my $tagphrase='NPB';

my $usevocab=1;

my @opts_usage=("creates rules from manlex.e,manlex.f dictionaries",
                ["outfile=s" => \$outfile_name,"Write output here (can be .gz)"],
                ["manlexpre=s" => \$manlexpre,"base part of manlex filenames (usually $MANLEXPRE_TEMPLATE.manlex.{e,f} - corresponding line by line)"],
                ["emanlex=s" => \$emanlex,"space separated english words without quotes - corresponds line by line with fmanlex"],
                ["fmanlex=s" => \$fmanlex,"space separated foreign words without quotes - corresponds line by line with emanlex"],
                ["tagfile=s" => \$tagfile,"Part of speech tags file from Brill tagger (first word = english word, second word = most common POS tag)"],
                ["default-tag=s" => \$tagdef,"Default part of speech tag if not found in {tagfile}"],
                ["constituent-tag=s" => \$tagphrase,"Nonterminal used for multi-word English phrases"],
                ["xrs-baseid=i" => \$baseid, "If set, start rule id=N here, increasing in distance from 0"],
                ["indicator-field=s" => \$indicator, "Name for binary indicator feature (value=1)"],
                ["prune-field=s" => \$pruneby, "Name for field used in pruning (useful for top-k lexical pruning)"],
                ["prune-value=s" => \$pruneval, "Value for field; {prune-field}={prune-value} is added to rules - useful to ensure that normal rules are always sorted as higher prob than lexicon rules in top-k lexical pruning"],
                ["logfile=s" => \$logfile_name,"(optional) write diagnostic messages here (as well as STDERR)"],
);

my $cmdline=&escaped_cmdline;
my ($usagep,@opts)=getoptions_usage(@opts_usage);

&outz_stderr($logfile_name);

info("COMMAND LINE:");
info($cmdline);
show_opts(@opts);

expand_opts(\@opts_usage,[$BLOBS_TEMPLATE,$BLOBS],[$SCRIPT_TEMPLATE,$FindBin::RealBin],[$MANLEXPRE_TEMPLATE,$manlexpre]);

### main program ################################################
&set_ioenc("raw"); # or encoding(gb..something)? #use is lexicaly scoped so:
use open IN => ':raw';
use open OUT => ':raw';

read_brill_pos_tags($tagfile) if $tagfile;

outz_stdout($outfile_name);

my $id_inc=(defined $baseid && $baseid < 0) ? -1 : 1;

open E,'<',$emanlex or die "$emanlex: $!";
open F,'<',$fmanlex or die "$fmanlex: $!";
my ($ne,$nf,$N)=(0,0,0);
while (defined(my $ephrase=<E>)) {
    my @ewords=split ' ',$ephrase;
    $ne+=scalar @ewords;
    my $fphrase=<F>;
    &debug($ephrase,$fphrase);
    die "$fmanlex had fewer lines than $emanlex" unless defined $fphrase;
    my @fwords=split ' ',$fphrase;
    $nf+=scalar @fwords;
    ++$N;
    print xrs_phrasal_rule(\@ewords,\@fwords,$tagphrase,$tagdef);
    if ($baseid) {
        print " id=$baseid";
        $baseid+=$id_inc;
    }
    print " $indicator=1";
    print " $pruneby=$pruneval" if $pruneby;
    print "\n";
}
die "$emanlex had fewer lines than $fmanlex" if defined(<F>);


&info_summary;
info("Total: $N phrasal rules, $ne english words, $nf foreign words (avg ".$ne/$N."/".$nf/$N.')');
