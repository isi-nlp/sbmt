#!/usr/bin/perl

while(<>) {
    if (/(.*) -> (.*)/) {
        my ($a,$b)=($1,$2);
        $a =~  s/(".*")/uc($1)/eg;
        $_="$a -> $b\n"
    }
    print;
}
