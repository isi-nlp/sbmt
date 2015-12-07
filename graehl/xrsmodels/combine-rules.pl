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
my $idfield='id';
my $dup_name;

my @opts_usage=("removes internal syntactic structure of xrs rules (leaving only root NT and leaves)",
                ["infile=s" => \$infile_name,"Take <file> as input (as well as the rest of ARGV)"],
                ["outfile=s" => \$outfile_name,"Write output here (can be .gz)"],
                ["logfile=s" => \$logfile_name,"(optional) write diagnostic messages here (as well as STDERR)"],
                ["dupfile=s" => \$dup_name,"(optional) write copies of multiply occuring rules here"],
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

my $dup_fh=openz_out($dup_name);
#my $outcmd=$outfile_name ? openz_out_cmd($outfile_name) : '';
#$outcmd=qq{sort --field-separator=\t | tr -d \t}.$outcmd if $sort;
outz_stdout($outfile_name);


&argvz;
my $lastrule;
my $lastattrs;
my $dupcount;
my $sep=" ### ";

sub finishrule {
    return unless defined $lastrule;
    print "$lastrule$sep$lastattrs";
    print " etree_variants=$dupcount";
    print "\n";
}

my $firstrep;

while (<>) { 
  if (/^(.*?) *$sep(.*)/o) { # for each rule line
    my ($rule,$attrs) = ($1,$2);
    if ($lastrule && $lastrule eq $rule) {
        ++$dupcount;
        if ($dup_fh) {
            print $dup_fh $firstrep if $firstrep;
            $firstrep=undef;
            print $dup_fh $_;
        }
    } else {
        $firstrep=$_;
        &finishrule;
        $dupcount=0;
    }
    ($lastrule,$lastattrs)=($rule,$attrs);
  } else { 
    print;			# if on the input is a comment, we pass it through.
    &info("Ignored line:",$_);
  }
}
&finishrule;

&info_runtimes;
