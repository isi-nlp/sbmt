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

my $outpre;
my $input;
my $tiburon="/nfs/amber/jonmay/public/software/tiburon/java/current/tiburon";
my $fdlog;
my $EPS=.00001;

my @options=(
q{STDIN = alternate tree,string pairs.  output one per line per pair, number of derivations using rules that explain pair (calls tiburon -l:tree -r:string -c rules)},
["outpre=s"=>\$outpre,"[outpre].l.1 .r.1 ,l.N, r.N, etc.  default = -rules"],
["rules=s"=>\$input,"Tiburon rules input here (required)"],
["tiburon=s"=>\$tiburon,"Tiburon command"],
["force-decoder-log=s"=>\$fdlog,"compare against force_decoder log for the same sentences: looks for 'parse forest has (loops excluded) 3.43597e+10 trees in 254 items connected by 334 edges.'"],
["epsilon=s"=>\$EPS,"relative difference allowed in comparing reported # of trees"],
);

&show_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
show_opts(@opts);

&argvz;

#&outz_stdout($outpre);

my $n=0;

$outpre=$input unless $outpre;

my @fdn=();
my $s;
my $n_parse_reports=0;

my $n_trees;

if ($fdlog) {
    my $FD=&openz($fdlog);
    while(<$FD>) {
        if (/^NBEST sent=(\d+) nbest=0 /) {
            $s=$1;
            if (defined $n_trees) {
                $fdn[$s]=$n_trees;
                $n_trees=undef;
            } else {
                fatal("Didn't get forest tree count before line: $_") unless /\bfailed-parse=1\b/;
                $fdn[$s]=0;
            }
        } elsif (/parse forest has \(loops excluded\) (\S+) trees in \S+ items connected by \S+ edges./) {
#            fatal("Didn't get sentence number (NBEST sent=n) before line: $_")
#            unless defined $s; 
            $n_trees=$1;
            ++$n_parse_reports;
        }
    }
    info_remember("force_decoder reported $n_parse_reports parsed forests, with maximum sent=$#fdn");
}


while(<>) {
    ++$n;
    my $tree=$_;
#    chomp $tree;
    my $string=<>;
#    &superchomp(\$string);
    my $l="$outpre.l.$n";
    my $r="$outpre.r.$n";
    open L,">",$l or die;
    open R,">",$r or die;
    print L $tree;
    print R $string;
    close L;
    close R;
    my $cmd="$tiburon -l:$l -r:$r -c $input";
    &info($cmd);
    my $cmdout=`$cmd`;
    print STDERR "\n$n:\n$cmdout";
    if ($cmdout =~ /(\S+)\s+derivations\s*\n/m) {
        my $nderiv=$1;
        if ($fdlog) {
            my $nfd=$fdn[$n];
            print "$nderiv";
            unless (&epsilon_equal($nfd,$nderiv,$EPS)) {
                warning("for sent=$n, # of derivations $nfd (force_decoder) != $nderiv (tiburon)") ;
                print " != $nfd";
            }
            print "\n";
        } else {
            print "$nderiv\n";
        }
    }
}

&info_summary;
