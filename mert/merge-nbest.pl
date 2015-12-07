#!/usr/bin/perl

while (<>) {
    chomp;
    if (!/^\s*(\d+)\s*\|\|\|\s*(.*)\s*$/) { die "nbest format error" }
    $id = $1; $hyp = $2;
    ${$hyps[$id]}{$hyp} = 1;
}

for ($id=0; $id <= $#hyps; $id++) {
    foreach $hyp (keys(%{$hyps[$id]})) {
        print "$id ||| $hyp\n";
    }
}
