#!/usr/bin/perl

my @bylen;

while (<>) {
    if (/\[(\d+),(\d+)\]=/) {
        my ($a,$b)=($1,$2);
        my $l=$b-$a;
        push @{$bylen[$l]},$_;
    }
}

for (@bylen) {
    for my $l (@$_) {
        print $l;
    }
    print "\n";
}
