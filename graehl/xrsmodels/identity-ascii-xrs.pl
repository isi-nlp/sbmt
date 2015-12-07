#!/usr/bin/env perl

#(graehl) create identity xrs rules given input tokenized foreign text.

use strict;
use warnings;
use Getopt::Long;

my $blobbase="/home/hpc-22/dmarcu/nlg/blobs";

### script info ##################################################
my $BLOBS;
BEGIN {
    $ENV{BLOBS}='/home/hpc-22/dmarcu/nlg/blobs' unless exists $ENV{BLOBS};
    $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
    $BLOBS=$ENV{BLOBS};
    my $libgraehl="$BLOBS/libgraehl/unstable";
    unshift @INC,$libgraehl if -d $libgraehl;
}

use FindBin;
use lib $FindBin::RealBin;

require "libgraehl.pl";
require "libxrs.pl";

### arguments ####################################################
my $BLOBS_TEMPLATE="{blobs}";

my $infile_name;
my $outfile_name;
my $fenc='raw';
my $tenc='raw';
my $CD='CD';
my $NN='NNP';
my $PUNC='SYM';
my $dopunc=0;
my $dotext=0;
my $doalpha=1;
my $donums=0;
my $baseid;
my $minlen=0;

my $cls_punc=q(!-/:-@\[-`{-~);
my $cls_num=q{0-9.,\-:};
my $cls_alpha=q{a-zA-Z\-.};
my $cls_text=$cls_alpha.$cls_num.$cls_punc;

my @opts_usage=("create identity xrs rules given input tokenized (plain) foreign text.",
                ["min-length=i" => \$minlen, "Only make rules for words at least this many characters"],
                ["infile=s" => \$infile_name,"Take <file> as input (as well as the rest of ARGV)"],
                ["outfile=s" => \$outfile_name,"Write output here (can be .gz)"],
                ["foreign-encoding=s" => \$fenc, "Encoding used by (space separated) foreign words file"],
                ["target-encoding=s" => \$tenc, "Encoding for output"],
                ["do-numbers!" => \$donums,"Create rules for numbers"],
                ["number-pos=s" => \$CD, "Part of speech used for numbers"],
                ["do-text!" => \$dotext,"Create rules for words that are (arbitrary) ASCII text"],
                ["do-alpha!" => \$doalpha,"Create rules for words that are (alphabetic) ASCII text"],
                ["text-pos=s" => \$NN, "Part of speech used except for numbers"],
                ["do-punc!" => \$dopunc, "Create rules for punctuation"],
                ["punc-pos=s" => \$PUNC, "Default part of speech used for purely-punctuation words (Note: single-character punctuation is handled specially, e.g. '('->-LRB- '\"'->'')"],
                ["xrs-baseid=i" => \$baseid, "If set, start rule id=N here, increasing in distance from 0"],
);

my $cmdline=&escaped_cmdline;
my ($usagep,@opts)=getoptions_usage(@opts_usage);

info("COMMAND LINE:");
info($cmdline);
show_opts(@opts);

expand_opts(\@opts_usage,[$BLOBS_TEMPLATE,$BLOBS]);

$doalpha=1 if $dotext;

### main program ################################################
my $enc=set_inenc($fenc);
use open IN => $enc;

if ($infile_name) {
    info("Adding infile = $infile_name");
    unshift @ARGV,$infile_name;
}
outz_stdout($outfile_name);
set_outenc($tenc);

my %rules;

sub create_rule
{
    my ($tag,$ident,$type)=@_;
    if (length $ident < $minlen) {
        info_remember_quiet("create_rule($type) shorter than $minlen characters - skipping");
    } else {
        info_remember_quiet("create_rule($type) - $tag created",": $tag($ident)");
        $rules{xrs_lexical_rule($tag,$ident)}=1;
    }
}

while (<>) {
    chomp;
    my @words=split;
    for (@words) {
        if (/^[$cls_punc]*$/) {
            my $puncpos=punc_pos($_);
            $puncpos=$PUNC unless defined $puncpos;
            info_remember_quiet("punctuation",": $puncpos($_)");
            create_rule($puncpos,$_,"punctuation") if $dopunc;
        } elsif (/^[$cls_num]*$/) {
            info_remember_quiet("number",": $CD($_)");
            create_rule($CD,$_,"number") if $donums;
        } elsif (/^[$cls_alpha]*$/) {
            info_remember_quiet("alpha",": $NN($_)");
            create_rule($NN,$_,"alpha") if $doalpha;
        } elsif (/^[$cls_text]*$/) {
            info_remember_quiet("text",": $NN($_)");
            create_rule($NN,$_,"text") if $dotext;
        } else {
            info_remember_quiet("foreign",": $_");
        }
    }
}

my $inc=(defined $baseid && $baseid < 0) ? -1 : 1;

for (sort keys %rules) {
    print $_;
    if ($baseid) {
        print " id=$baseid";
        $baseid+=$inc;
    }
    print " identity=1";
    print "\n";
}

&info_summary;
