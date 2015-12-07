#!/bin/env perl

use strict;
use warnings;

while(<STDIN>){
    if(/^(.*?) -> (.*?) ### /){
        my $lhs=$1;
        my $rhs=$2;

        my $n_rhs_lex = 0;
        for (split /\s+/, $rhs) {
            if(not /^x\d+/) { $n_rhs_lex++;}
        }
        if($n_rhs_lex == 0){
            while($lhs=~/\("(\S+?)"\)/g){
                print $1 . "\n";
            }
        }
    }
}



