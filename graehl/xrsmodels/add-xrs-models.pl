#!/usr/bin/env perl

#(graehl) creates xrs rule file attribute unigram LM prob + # of unknown
#English(lhs) words

#UNANSWERED QUESTIONS:

## min prob useful for unigram lm? ( we have oov )

## interpolate bins in logspace or probspace (doing probspace for easier
  ## handling of "0" boundary



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
require "permute.pl";

### arguments ####################################################
my $BLOBS_TEMPLATE="{blobs}";

my $lmfile_name="$BLOBS_TEMPLATE/ngram-files/news-trained/v1/e-lm.SRILM";
my $maxcost;
my $at_numclass=1;
my $lmfield='unigram_lm';
my $infile_name;
my $outfile_name;
my $permfieldname='nonmonotone';
my $countfield="count-bin";
my $binbounds='0,1,4,16';
my $sourcecountfield="unit_count_it1";
my $addlm=1;
my $addnonmon=1;
my $addcountbin=1;
my $precision=4;
my $disttype='steps';
my $logfile_name;
my $dbgunivocab;

my @opts_usage=("creates xrs rule file with additional attributes",
                ["infile=s" => \$infile_name,"Take <file> as input (as well as the rest of ARGV)"],
                ["outfile=s" => \$outfile_name,"Write output here (can be .gz)"],
                ["logfile=s" => \$logfile_name,"(optional) write diagnostic messages here (as well as STDERR)"],
                ["precision=s" => \$precision,"Number of decimal places to keep (more gives bigger files without any useful discriminative information)"],

                "For unigram LM prob + # of unknown English(lhs) words",
                ["add-lm!" => \$addlm,""],
                ["lm-fieldname=s" => \$lmfield,"append {unigram-fieldname}=e^-1.3 probs and {unigram-fieldname}_oov=N number of unseen words"],
                ["lm=s" => \$lmfile_name,"Load SRILM file (possibly compressed) from here"],
                ["maxcost-log10=s" => \$maxcost,"Maximum positive cost (negative log10 probability) for unigram p(e) - note: shouldn't be necessary - unseen words already add to the oov count: fieldname-oov=N"],
                ["at-numclass!" => \$at_numclass,"replace all digits by '\@' for ngram scoring - recommended if your LM does the same :)"],
                ["dbg-show-unigram-vocab!" => \$dbgunivocab,"dump unigram vocabulary and exit (debugging)"],

                "For nonmonotone (amount of reordering): append nonmonotone_type=x and nonmonotone_type_norm=y, where x is a sum of distances from the original position, ignoring lexemes, and y is the normalized [0..1] version",
                ["add-nonmonotone!" => \$addnonmon, ""],
                ["permutation-distance-type=s" => \$disttype,"type of permutation distance (bigram,steps,jumps,distance)"],
                ["nonmonotone-fieldname=s" => \$permfieldname,"fields named {fieldname}_{distance-type} and {fieldname}_{distance-type}_norm"],

                "For fuzzy-count-bin-membership vectors (linear-IN-PROBSPACE interpolated between two nearest neighbors) (tunable smoothing), e.g. if bin-count-control-points=\"0.0001,.5,e^0\" and original count=.75, then membership is (0,.5,.5); if original count=e^15, then membership is (0,0,1), if original count=e^-11, membership is(1,0,0):",
                ["add-count-bin!" => \$addcountbin, ""],
                ["source-count-fieldname=s" => \$sourcecountfield, "field to read source count from"],
                ["bin-control-points=s" => \$binbounds, "(comma separated) count values that determine the control points/bin boundaries"],
                ["count-bin-fieldname=s" => \$countfield,"append as{count-bin-fieldname}_x=a, {name}_y={1-a}, where x and y were the two closest {bin-control-points}"],
);

my $cmdline=&escaped_cmdline;
my ($usagep,@opts)=getoptions_usage(@opts_usage);

&outz_stderr($logfile_name);

set_default_precision($precision);

info("COMMAND LINE:");
info($cmdline);
show_opts(@opts);

expand_opts(\@opts_usage,[$BLOBS_TEMPLATE,$BLOBS]);

my %disttypes=(
               'bigram'=>\&norm_bigram_err,
               'steps'=>\&norm_steps_to_identity,
               'jumps'=>\&norm_moves_to_identity,
               'distance'=>\&norm_distance_from_identity,
);

$usagep->("Unknown --permutation-distance-type $disttype - should be one of (bigram,steps,jumps,distance)") unless exists $disttypes{$disttype};
my $distproc=$disttypes{$disttype};

my $lmoovfield=$lmfield."_oov";
$permfieldname.="_$disttype";
my $permnormfield=$permfieldname."_norm";

### main program ################################################
if ($infile_name) {
    info("Adding infile = $infile_name");
    unshift @ARGV,$infile_name;
}
outz_stdout($outfile_name);

# for UNIGRAM LM
my %unigrams;
if ($addlm) {
    info("reading LM $lmfile_name...");
    my $lmf=openz($lmfile_name);
    read_srilm_unigrams($lmf,\%unigrams);
    my $nuni=keys %unigrams;
    &info("Read $nuni english words from LM file $lmfile_name - attaching attributes $lmfield and $lmoovfield...");
    if ($dbgunivocab) {
        info("LM unigrams:");
        for (sort keys %unigrams) {
            print "$_\n";
        }
        exit;
    }
}
my $minprob=-$maxcost if defined $maxcost;


# for COUNT BINS
my @realbins;
my @binnames;
if ($addcountbin) {
    my @bins=split /,/,$binbounds;
    @realbins=map { getreal($_) } @bins;
    @binnames=map { s/\^/_/g; $countfield.'_'.$_ } @bins;
    info("Creating fuzzy count bins: ",join(',',@binnames));
    &debug('realbins',\@realbins);
}

&argvz;
while (<>) { 
  if (/^(([^( ]+)\(.*\)) -> (.*) ### (.*)/) { # for each rule line
    my $lhs = $1;		# lhs of rule
    my $nt = $2;		# non terminal at top
    my $rhs = $3;		# rhs of rule
    my $attributes = $4;	# attribute strings

    my $oldrule = $_;	# the whole line
    my $newrule=$_;
    chomp $newrule;

#    my ($lmfeats,$monfeats,$countfeats)=('','',''); #potentially more efficient
# than repeatedly appending to keep separate?  who knows with PERL

    if ($addlm) {
        my $logprob=0;
        my $oov=0;
        while ($lhs =~ /\"(.+?)\"/g) {
            my $word=$1;
            $word =~ s/\d/\@/g if $at_numclass;
#            &debug("word",$word,"logprob",exists $unigrams{$word} ? $unigrams{$word} : '<OOV>');
            if (exists $unigrams{$word}) {
                if (defined $maxcost) {
                    my $lp=$unigrams{$word};
                    $logprob += ($lp < $minprob ? $minprob: $lp);
                } else {
                    $logprob +=$unigrams{$word};
                }
            } else {
                ++$oov;
            }
        }
        $newrule .= " $lmfield=".log10_to_ehat($logprob) if $logprob;
        $newrule .= " $lmoovfield=$oov" if $oov;
    }

    if ($addnonmon) {
        my @rhsvars; # we assume that rhs subtrees aren't completely deleted
                      # (although they very well could be!)
        push @rhsvars,$1 while($rhs =~ /\bx(\d+)\b/g);
#        &debug("rhsvars",@rhsvars);
        if (scalar @rhsvars) {
            my ($normpermdist,$permdist)=map { real_prec($_) } $distproc->(@rhsvars);
            $newrule .= " $permfieldname=$permdist" if $permdist;
            $newrule .= " $permnormfield=$normpermdist" if $normpermdist;
        }
    }

    if ($addcountbin) {
        my $count=getfield($sourcecountfield,$attributes);
        &debug("got count $countfield=",\$count," from $attributes");
        if (defined($count)) {
            my $realcount=getreal($count);
            my ($basei,$tonext)=nearest_two_linear($realcount,\@realbins);
#            &debug("set membership: $realcount is in $basei,$tonext",$binnames[$basei],$realbins[$basei]);
            $tonext=real_prec($tonext);
            my $tobase=real_prec(1-$tonext);
#            &debug($realcount,$realbins[$basei],$tobase,$realbins[$basei+1],$tonext);
            $newrule .= " $binnames[$basei]=$tobase" if ($tobase);
            $newrule .= " $binnames[$basei+1]=$tonext" if ($tonext);
        }
    }

    print $newrule,"\n";
  } else { 
    print;			# if on the input is a comment, we pass it through.
    &info("Ignored line:",$_);
  }
}

&info_runtimes;
