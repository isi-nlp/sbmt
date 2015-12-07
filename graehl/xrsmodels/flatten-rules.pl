#!/usr/bin/env perl

#(graehl) removes internal syntactic structure of xrs rules (leaving only root NT and leaves)

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

my $infile_name;
my $outfile_name;
my $logfile_name;
my $tabsep;
my $sort;
my $outjustlhs;
my $outjustrule;
my $idfield='id';
my $flatten=1;

my @opts_usage=("removes internal syntactic structure of xrs rules (leaving only root NT and leaves)",
                ["infile=s" => \$infile_name,"Take <file> as input (as well as the rest of ARGV)"],
                ["outfile=s" => \$outfile_name,"Write output here (can be .gz)"],
                ["lhsoutfile=s" => \$outjustlhs,"Write just rule lhs here (can be .gz)"],
                ["ruleoutfile=s" => \$outjustrule,"Write rules without attributes here (can be .gz)"],
                ["logfile=s" => \$logfile_name,"(optional) write diagnostic messages here (as well as STDERR)"],
                ["flatten!" => \$flatten,"flatten rule lhs (-noflatten leaves them alone)"],
);

my $cmdline=&escaped_cmdline;
my ($usagep,@opts)=getoptions_usage(@opts_usage);

&outz_stderr($logfile_name);

info("COMMAND LINE:");
info($cmdline);
show_opts(@opts);

expand_opts(\@opts_usage,[$BLOBS_TEMPLATE,$BLOBS]);

### main program ################################################
if ($infile_name) {
    info("Adding infile = $infile_name");
    unshift @ARGV,$infile_name;
}

#my $outcmd=$outfile_name ? openz_out_cmd($outfile_name) : '';
#$outcmd=qq{sort --field-separator=\t | tr -d \t}.$outcmd if $sort;
outz_stdout($outfile_name);
my $outlhs=openz_out($outjustlhs) if $outjustlhs;
my $outrule=openz_out($outjustrule) if $outjustrule;

my $sizepre=0;
my $sizeflat=0;

&argvz;
while (<>) { 
  if (/^([^( ]+)\((.*)\)( -> .* )(### .*)/) { # for each rule line
    my $nt = $1;		# lhs of rule
    my $children = $2;		# non terminal at top
    my $rest = $3;		# rhs of rule
    my $attr = $4;
    my $finalchildren;
    if ($flatten) {
        my @leaves;
        while ($children =~ /(\".+?\"|x\d+(?:\:[^ )]*)?)/g) {
            push @leaves,$1;
        }
        $finalchildren=join(' ',@leaves);
    } else {
        $finalchildren=$children;
    }
    $sizepre+=length($children);
    $sizeflat+=length($finalchildren);
    my $lhs="$nt($finalchildren)";
    print $outlhs "$lhs\n" if ($outlhs);
    my $rule=$lhs.$rest;
    print $outrule "$rule\n" if ($outrule);
    print "$rule$attr\n";
  } else { 
    print;			# if on the input is a comment, we pass it through.
    &info("Ignored line:",$_);
  }
}

&info_runtimes;
&info("# bytes in english trees: $sizepre (before), $sizeflat (flattened)");
