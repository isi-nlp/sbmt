#!/usr/bin/env perl
# input (STDIN) is
# sysname SENT #n HYP #m HYPOTHESIS <<< ||| repeats on ties ||| ... >>>
# sorted by n
# sysname may be omitted i.e. this can process gen_nbests
# output is 1bests hypothesis (m=1) one per line, with kth line being SENT #k. 
# stderr lists winning system (with ties) ... can feed to jens_percentage
# this means blank lines are inserted when there are gaps

# originally: jens_bestk_lines #lines, padded blank lines at end as well

use strict;

# set -1 arg if you're passing nbest lists and want to get the 1best out.  HYP n
# in mbr output just means that the selected best was originally in that
# position 

#my $just1=scalar @ARGV && $ARGV[0]=='-1';
#shift if $just1;


my $mbrhyp=qr/(?:(\S+) )?SENT (\d+) HYP (\d+) \S+ (.*?)/;
my $line = 0;
while ( <> ) {
#FIXME: file format means you can't have ||| in your translations?
    if ( /^$mbrhyp( \|\|\| $mbrhyp)*$/o ) {
        my ($sys,$sent,$best_from_sys_n,$hyp)=($1,$2,$3,$4);
        next unless $sys || $best_from_sys_n==1;
        print "\n" while (++$line < $sent);
        print "$hyp\n";
        next unless defined $sys;
        while ( /.*? \|\|\| (\S+) SENT \d+ HYP \d+ /g ) { 
            $sys = "$sys tie with $1"; 
        } 
        print STDERR "$sys\n";
    }
}
