#!/usr/bin/perl

# eliminate all unary rules except lexical (F word) and TOP(x0:GLUE), TOP(x0:S)

my $nt="([^ ()]*)";

while(<>) {
    if (/$nt\(x0:$nt\) -> x0 ###/ && $1 ne 'TOP') {
        print STDERR "# removed $1($2)\n";
    } else {
        print;
    }
}
