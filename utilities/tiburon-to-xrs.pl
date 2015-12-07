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

my $output='-';
my $forestem='';
my $input='';
my $tiburon="java -Xmx1000m -jar $scriptdir/XRSToDecoder.jar -s -i";
my $saved='';
my $weightname='';
my $tiegroups;
my $id_origin=2;

my @options=(
q{read Tiburon format tree->string transducer rules, write nearly equivalent (root=state) decoder xrs rules on output - binarize with "itg_binarizer -s" for force_decoder.  Output rules have a tie-group=N feature when the input rules were tied},
["output=s"=>\$output,"write xrs rules here"],
["input=s"=>\$input,"Tiburon rules input here (required)"],
#["tiegroups"=>\$tiegroups,"write to this file, one per line, space separated, the ids of rules in tied groups, starting with lowest id"],
["xrs-id-origin=s"=>\$id_origin,"start counting at id=N for the top->start rule"],
["cmd-tiburon-to-xrs=s"=>\$tiburon,"command e.g. 'java -jar XRSToDecoder.jar -i' [input], that is, --input is passed as an argument"],
["saved-tiburon-to-xrs=s"=>\$saved,"previous output from tiburon-to-xrs-cmd input (ingore --input)"],
["fieldname=s",\$weightname,"attribute name for tiburon rule weight"],
);

&show_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
show_opts(@opts);

&argvz;

&outz_stdout($output);

my %tied;

my $X;

if ($saved) {
    $X=openz($saved);
} else {
    $input=shift unless $input;
    &require_file($input);
    print STDERR "$tiburon $input";
    open($X,"$tiburon $input |") or die;
}


my $id=$id_origin;

while (<$X>) {
# it's \s*(#\s+weight)?\s*(@\s+id)$ where weight is some double and id is some
# int, but yes. in case i got the regexp wrong, I mean after the rule "optional
# whitespace then, if a weight, # followed by some space followed by the weight,
# then, if a tie, @ followed by some space followed by the tie. both are
# optional; if both are present the weight comes first.
    my $FP=&number_re;
    my $ID=&integer_re;

#    &debug($_);
    if (s/\s*(?:#\s+($FP))?\s*(?:\@\s+($ID))?$//) {
        my $weight=$1;
        my $tiegroup=$2;
#        my $fid=$id;
#        if (defined $tiegroup && $tiegroups) {
#            push_ref(\$tied{$2},$id);
#            $fid=$tied{$2}->[0];
#        }
# print " tied-id=$fid";
        chomp;
        print "$_ ### id=$id";
        print " tie-group=$tiegroup" if (defined $tiegroup);
        print " $weightname=$weight" if ($weight and $weightname);
        print "\n";
        ++$id;
    }
}

my $F;

if ($tiegroups) {
    $F=openz_out($tiegroups);
    while (my ($grp,$ids)=each %tied) {
        next unless ($ids and scalar @$ids > 1); # could be > 1 but then you
                                                 # lose the original singleton
                                                 # tie ids
        print $F (join ' ',@$ids),"\n";
    }
}
