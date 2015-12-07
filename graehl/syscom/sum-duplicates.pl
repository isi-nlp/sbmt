#!/usr/bin/env perl
use strict;

use File::Basename qw(dirname basename);

BEGIN {
 	push @INC, dirname($0);
}

require "libgraehl.pl";

my $base=10;
my $out;
my $ndup=0;
my $ntot=0;

my @options=(
q{sum duplicate hypotheses for the same sentence, renumbering ranks so no gaps (but not sorting for new best)
input is lines of SENT 1 HYP 1 -204.077934152 britain ...
base^-204 is the prob that gets summed
 },
["base=s"=>\$base,"this is the base for the exponents (scores)"],
["$out=s"=>\$out,"write output here"],
);

my $cmdline=&escaped_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
info("COMMAND LINE:");
info($cmdline);
show_opts(@opts);

### main program ################################################
my $lnbase=log($base);

outz_stdout($out) if $out;
&argvz;


my $lastid;
my %hyps;
my @hyps;


sub outs {
    my $sentpre="SENT $lastid ";
    my $h=0;
    for my $hyp (@hyps) {
        my $l=$hyps{$hyp};
        my $score=$l->[0];
        die unless defined $score;
        $score=logadd_b($lnbase,$score,$l->[$_]) for (1..$#$l);
#        &count_info_gen("$#$l duplicates");
        &log_numbers("$#$l duplicates sum=$score,added=".($score-$l->[0])) if $#$l;
        print $sentpre,"HYP ",$h++," ",$score," ",$hyp,"\n";
    }
}

while (<>) {
    if (/^(SENT (\d+) )HYP \d+ (\S+) (.*)$/) {
        ++$ntot;
        my ($sentpre,$id,$score,$hyp)=($1,$2,$3,$4);
        if ($lastid!=$id) {
            &outs;
            $lastid=$id;
            %hyps=();
            @hyps=();
        }
        if (exists $hyps{$hyp}) {
            push @{$hyps{$hyp}},$score;
            ++$ndup;
        } else {
            push @hyps,$hyp;
            $hyps{$hyp}=[$score];
        }
    }
}
&outs;

info("$ntot total input nbests, $ndup duplicates summed. NOTE: nbests are not re-sorted\n");

&all_summary;
