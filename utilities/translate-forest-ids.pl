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

my $weightname='';

my $weightout;
my $tied_lines;
my $norms;
my $forest_map;
my $output;

my $param_id_origin=2;

my @options=(
q{rewrite rule ids in a forest-em forest (leave alone the #N backrefs, but modify all other numbers according to a forest-id map},
  ["forest-translate-in=s"=>\$forest_map,"read forest-tie 'xrs-id param-id' table here.  note: identity (xrs-id = param-id) lines omitted"],
  ["output=s"=>\$output,"output here"]
);

&show_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
show_opts(@opts);

&argvz;
&outz_stdout($output);

my %x_to_p=();

&read_hash($forest_map,\%x_to_p);

while(<>) {
    s/(?<!\#)(-?\d+)/hash_lookup_default(\%x_to_p,$1,$1)/goe;
    print;
}
