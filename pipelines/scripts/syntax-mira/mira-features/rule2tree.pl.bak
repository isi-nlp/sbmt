#!/usr/bin/env perl
use strict;
use warnings;

sub replace {
    my $v = shift;

    if($v =~ /\("(\S+?)"\)/){
        my $u=$1;
        $u=~s//-RRB-/g;
        $u=~s/\)/-RRB-/g;
        $u=~s/\(/-LRB-/g;
        $v="(\"$u\")";
    }

    return "$v";
}

while(<STDIN>){
    if(/^(.*?) -> .*? ###.* id=(\d+)/){
        my $lhs=$1;
		my $rid=$2;

        $lhs=~s/-RRB--\d+/-RRB-/g;
        $lhs=~s/-LRB--\d+/-LRB-/g;
        $lhs=~s/(\("\S+?"\))/&{replace}($1)/ge;
        #$lhs=~s/\("\)"\)/\(-RRB-\)/g;
        $lhs=~s/\("(\S+?)"\)/\($1\)/g;
        $lhs=~s/([^\(\) ]+)\(/\($1 /g;
        $lhs=~s/x\d+:([^ \)\(]+)/\($1 aa\) /g;
        $lhs=~s/\s+/ /g;
        $lhs=~s/([^\)\( ]+) \)/$1\)/g;
        # change leaves
        print $rid . " " . $lhs . "\n";



    }
}
