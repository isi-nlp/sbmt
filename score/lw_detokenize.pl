#!/usr/bin/perl -w

#Copyright (c) 2005, Language Weaver Inc, All Rights Reserved.
#
#This software is provided 'as is' with no explicit or implied warranties.
#
#LW DeTokenizer

use strict;

#Merge Direction: Token => [occurence match,reset occurence after match]

my %_MergeRight = ( 
		 '"' => [1,0],
		 '(' => [1,1],
		 '[' => [1,1],
		 '$' => [1,1],
		 '`' => [1,0],
		 '{' => [1,1],
		 '-@' => [1,1],
		 ':@' => [1,1],
		 '/@' => [1,1],
		 );

my %_MergeLeft = ( 
		'.' => [1,1],
		'?' => [1,1],
		'!' => [1,1],
		'"' => [2,1],
		',' => [1,1],
		')' => [1,1],
		']' => [1,1],
		';' => [1,1],
		':' => [1,1],
		'\'' => [1,1],
		'`' => [2,1],
		'%' => [1,1],
		'}' => [1,1],
		'\'t' => [1,1],
		'\'T' => [1,1],
                'n\'t' => [1,1],
                'N\'T' => [1,1],
		'\'s' => [1,1],
		'\'re' => [1,1],
		'\'ll' => [1,1],
		'\'m' => [1,1],
		'\'d' => [1,1],
		'\'ve' => [1,1],
                '\'S' => [1,1],
                '\'RE' => [1,1],
                '\'LL' => [1,1],
                '\'M' => [1,1],
                '\'D' => [1,1],
                '\'VE' => [1,1],
		'TH' => [1,1],
		'RD' => [1,1],
		'ST' => [1,1],
                'th' => [1,1],
                'rd' => [1,1],
                'st' => [1,1],
		'@-' => [1,1],
		'@/' => [1,1],
		':-' => [1,1],
		);

my %_MergeBoth = ( 
		'@-@' => [1,1],
		'@:@' => [1,1],
		'@/@' => [1,1],
		'*' => [1,1],
		'^' => [1,1],
		);

my ($line,$w,$a);

while($line=<stdin>){
    chomp($line);
    $line =~ s/(ca)n (n't)/$1 $2/ig;
    $line =~ s/([Ww])I[lL][lL] ([nN]'[tT])/$1O $2/g;
    $line =~ s/(w)ill (n't)/$1o $2/ig;
    my @words = split(/ +/,$line);
    my $str = "";
    my %occurence = ();
    my $prefix="";
    foreach $w (@words){
	$occurence{$w}++;
	if (defined($a=$_MergeLeft{$w}) && ($a->[0] == $occurence{$w})){
	    $str .= $w;
	    $prefix=" ";
	    $occurence{$w} = 0 if ($a->[1]);
	}
	elsif (defined($a=$_MergeRight{$w}) && ($a->[0] == $occurence{$w})){
	    $str .= "$prefix"."$w";
	    $prefix="";
	    $occurence{$w} = 0 if ($a->[1]);
	}
	elsif (defined($a=$_MergeBoth{$w}) && ($a->[0] == $occurence{$w})){
	    $str .= "$w";
	    $prefix="";
	    $occurence{$w} = 0 if ($a->[1]);
	}
	else{
	    $str .= "$prefix"."$w";
	    $prefix=" ";
	    $occurence{$w}=0;
	}
    }    
    $str =~ s/\@([\-\:\/])/$1/g;
    $str =~ s/([\:\-\/])\@/$1/g;
    $str =~ s/(\b\d+)\s+s(\b)/$1s$2/g;
    print "$str\n";
}
