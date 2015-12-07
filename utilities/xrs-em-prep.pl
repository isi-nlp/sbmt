#!/usr/bin/env perl
#
# Author: graehl

use strict;
use warnings;
use Getopt::Long;
use Tie::File;

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
my $param_out;
my $norms;
my $forest_map;

my $param_id_origin=2;

my @options=(
q{xrs rules on input with optional "tie-group=N" attribute: figure out and write mapping between xrs id and params (two versions: xrs id -> param (to amend decoder forests before EM; and the inverse, a line-by-line id=N by parameter id, to get line by line weights corresponding to original xrs input lines, from forest-em).  Also writes by-root normalization groups for forest-em; note: the groups are organized by parameter id, not xrs id.},
["param-id-out=s"=>\$param_out,"write (aligned with input rules) ' id=N' lines with N being the param id, not the original xrs id"],
  ["forest-translate-out=s"=>\$forest_map,"write forest-tie 'xrs-id param-id' table here.  note: identity (xrs-id = param-id) lines omitted"],
  ["normgroups-out=s"=>\$norms,"write (parameter id, forest-em) joint normalization groups here"],
#["tiegroups"=>\$tiegroups,"write to this file, one per line, space separated, the ids of rules in tied groups, starting with lowest id"],
#["id-origin=s"=>\$param_id_origin,"start EM parameters at N"],
#["weight-fieldname=s",\$weightname,"attribute name for initial rule weight"],
#["initial-weight-out=s",\$weightout,"file for initial weights from weight-fieldname"],
);

&show_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
show_opts(@opts);

&argvz;

my $P=openz_out($param_out);
my $N=openz_out($norms);
my $F=openz_out($forest_map);

my %tied;
my %ng;

my $X;

my $next_pid=$param_id_origin;
my $ID=&integer_re;

sub badline {
    die ("xrs line missing expected ' ### ... id=N ...': $_");
}


# figure out and write mapping between xrs id and params
while (<>) {
    &badline unless (/^([^(]+)\(.* ### .*\bid=(-?\d+)\b/);
    my ($nt,$xid)=($1,$2);
    my $pid;
    if (/ ### .*\btie-group=($ID)\b/o) {
        my $tid=$1;
        $tied{$tid}=$next_pid++ unless exists $tied{$tid};
        $pid=$tied{$tid};
    } else {
        $pid=$next_pid++;
    }
    print $P "id=$pid\n";
    print $F "$xid $pid\n" unless $xid == $pid;
    push @{$ng{$nt}},$pid;
}

# write normgroups
print $N "(\n";

print $N " ( $_ )\n" for (1..$param_id_origin-1);

while (my ($nt,$ids)=each %ng) {
    my %seen=();
    print $N " (";
    for (@$ids) {
        print $N ' ',$_  unless $seen{$_}++;
#tied parameters should only appear once in normgroup; forest-em would weight
#them N times too much in denominator for normalization otherwise
    }
    print $N " )\n";
}
print $N ")\n";
