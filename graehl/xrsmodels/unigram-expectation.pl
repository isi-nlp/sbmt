#!/usr/bin/env perl

#(graehl) measures average per-word log10(prob) and #oov for a plain text input, using an SRILM

use strict;
use warnings;

my $blobbase="/home/hpc-22/dmarcu/nlg/blobs";

### script info ##################################################
use FindBin;
use lib $FindBin::RealBin;
my $BLOBS;

BEGIN {
    $ENV{BLOBS}='/home/hpc-22/dmarcu/nlg/blobs' unless exists $ENV{BLOBS};
    $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
    $BLOBS=$ENV{BLOBS};
    my $libgraehl="$BLOBS/libgraehl/unstable";
    push @INC,$libgraehl if -d $libgraehl;
}

require "libgraehl.pl";

### arguments ####################################################
my $BLOBS_TEMPLATE="{blobs}";

my $lmfile_name="$BLOBS_TEMPLATE/ngram-files/news-trained/v1/e-lm.SRILM";
my $maxcost;
my $at_numclass=1;
my $lmfield='unigram_lm';
my $infile_name;
my $outfile_name;
my $precision=4;
my $logfile_name;

my @opts_usage=("measures average per-word log10(prob) and #oov for a plain text input, using an SRILM",
                ["infile=s" => \$infile_name,"Take <file> as input (as well as the rest of ARGV)"],
                ["outfile=s" => \$outfile_name,"Write output here (can be .gz)"],
                ["logfile=s" => \$logfile_name,"(optional) write diagnostic messages here (as well as STDERR)"],
                ["precision=s" => \$precision,"Number of decimal places to keep (more gives bigger files without any useful discriminative information)"],

                "For unigram LM prob + # of unknown English(lhs) words",
                ["lm-fieldname=s" => \$lmfield,"append {unigram-fieldname}=e^-1.3 probs and {unigram-fieldname}_oov=N number of unseen words"],
                ["lm=s" => \$lmfile_name,"Load SRILM file (possibly compressed) from here"],
                ["maxcost-log10=s" => \$maxcost,"Maximum positive cost (negative log10 probability) for unigram p(e) - note: shouldn't be necessary - unseen words already add to the oov count: fieldname-oov=N"],
                ["at-numclass!" => \$at_numclass,"replace all digits by '\@' for ngram scoring - recommended if your LM does the same :)"],
);

my $cmdline=&escaped_cmdline;
my ($usagep,@opts)=getoptions_usage(@opts_usage);

&outz_stderr($logfile_name);

set_default_precision($precision);

info("COMMAND LINE:");
info($cmdline);
show_opts(@opts);

expand_opts(\@opts_usage,[$BLOBS_TEMPLATE,$BLOBS]);

my $lmoovfield=$lmfield."_oov";

### main program ################################################
if ($infile_name) {
    info("Adding infile = $infile_name");
    unshift @ARGV,$infile_name;
}
outz_stdout($outfile_name);

# for UNIGRAM LM
my %unigrams;
info("reading LM $lmfile_name...");
my $lmf=openz($lmfile_name);
read_srilm_unigrams($lmf,\%unigrams);
my $nuni=keys %unigrams;
info("Read $nuni english words from LM file $lmfile_name - attaching attributes $lmfield and $lmoovfield...");
my $minprob=-$maxcost if defined $maxcost;

&argvz;
my ($N,$log10,$oov,$Nlines)=(0,0,0,0);

while (<>) {
    ++$Nlines;
    my @words=split;
    for my $word (@words) {
        ++$N;
        $word =~ s/\d/\@/g if $at_numclass;
        if (exists $unigrams{$word}) {
            if (defined $maxcost) {
                my $lp=$unigrams{$word};
                $log10 += ($lp < $minprob ? $minprob: $lp);
            } else {
                $log10 +=$unigrams{$word};
            }
        } else {
            info_remember("Unknown: $word");
            ++$oov;
        }
    }
}
my $avglogprob=real_prec($log10/$N);
my $avgoov=real_prec($oov/$N);
my $avgwords=real_prec($N/$Nlines);
info("#words: $N");
info("#words/line: $avgwords");
info("log10(prob)/word: $avglogprob");
info("#oov/word: $avgoov");

print "text-length=$avgwords $lmfield=".log10_to_ehat($avglogprob)." $lmoovfield=".real_prec($avgoov)."\n";

&info_summary;
&info_summary(\*STDOUT);
