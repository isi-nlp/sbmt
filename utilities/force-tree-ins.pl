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

BEGIN {
   $scriptdir = &File::Basename::dirname($0);
   ($scriptname) = &File::Basename::fileparse($0);
    unshift @INC, $scriptdir;
}

require "libgraehl.pl";

### arguments ####################################################

my $verbose=$ENV{DEBUG};

my $outpre='tree-force';
my $strip;
my $xrs;

my $numchunk=1;

my $param_id_origin=2;
my $decode;
my $decode_args='';
my $dummytree='FAILED("parse")';

my $dryrun;

my @options=(
q{create binary rule files and instruciton files for force decoding (tree -> string).  alternating tree,string lines on input},
 ["xrs-in=s"=>\$xrs,"xrs rules here"],
 ["strip-xrs-state!"=>\$strip,"remove xrs state (root of lhs tree) for purpose of tree constraint"],
#["num-chunks=s"=>\$numchunk,"number of separate instruction files for parallel
#decode"], 
["output-prefix=s"=>\$outpre,"filenames start with this ... prefix.gar.gz, prefix.ins"],
["decode!"=>\$decode,"run force_decoder immediately after, with forests to prefix.forests.gz"],
["args-force-decoder=s"=>\$decode_args,"additional arguments for --decode"],
["dryrun!"=>\$dryrun,"don't run binarize or decode, just echo commands"],
["dummy-leaf-tree=s"=>\$dummytree,"replace any tree that is just '0' with this (parser uses that for a failed parse)"]
);

sub run_shell {
    if ($dryrun) {
        print STDERR "DRYRUN:\n",@_,"\n";
    } else {
        system(@_);
    }
}


&show_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
show_opts(@opts);

&argvz;

require_file($xrs);
my $binarize=&which("itg_binarizer");

my ($dirpre,$filepre)=dir_file($outpre);

my $brf="$filepre.brf.gz";

my $brf_full="$dirpre$brf";

my $strip_arg = $strip ? "--stateful-etree-string" : "--etree-string";

my $bincmd="$binarize $strip_arg --left-right-backoff --input=$xrs --output=$brf_full";

&info("Binarizing: ",$bincmd);

&run_shell($bincmd);

my $n=0;

my $insfile="$outpre.ins";

&info("Writing instruction file: ",$insfile);

my $INS=openz_out("$insfile");

print $INS "load-grammar brf \"$brf\";\n";

while(<>) {
    ++$n;
    my $tree = $_;
    chomp $tree;
    &info("Tree #$n: ",$tree) if $verbose;
    my $string=<>;
    &superchomp(\$string);
    $string =~ s/(^| )"/$1/go;
    $string =~ s/"( |$)/$1/go;
    
    die unless defined $string;
    &info("String #$n: ",$string) if $verbose;

    $tree = $dummytree if $tree =~ /^\s*0\s*$/; #unless /\(.*\)/ 

#FIXME: does force_decoder xml-tag the tree for me?
    print $INS <<EOF

force-decode {
 source: $string
 target: $tree
}
EOF
}

my $fd="force_decoder";

if ($decode) {
    $fd=which($fd);
}

my $forestout="$outpre.forests.gz";
my $log="$outpre.decode.log";
my $fdcmd="$fd --force-treebank-etree --instruction-file=$insfile --multi-thread --print-forest-em-file=$forestout --print-forest-em-cons-id=1 --print-forest-em-binary=0 $decode_args 2>&1 | tee $log";
&info("Force decode:\n",$fdcmd);

if ($decode) {
    &run_shell($fdcmd);
}
