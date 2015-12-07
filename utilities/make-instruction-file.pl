#!/usr/bin/env perl

use FileHandle;
use Getopt::Long;
use strict;

my $sentence_filename = "-";
my $instruction_filename = "-";
my $grammar_filename = "";

my $n=1;

GetOptions( "corpus:s"  => \$sentence_filename
          , "archive:s" => \$grammar_filename
          , "output:s"  => \$instruction_filename
          , "n:i" => \$n);
          
my $sent_fh = new FileHandle "< $sentence_filename";
my $ins_fh = new FileHandle "> $instruction_filename";

print $ins_fh  "load-grammar archive \"$grammar_filename\";\n" 
    unless ($grammar_filename eq "");


while(<$sent_fh>) {
    chomp;
    my $l=$_;
    print $ins_fh "decode $l \n" for (1..$n);
}

