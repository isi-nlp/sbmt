#!/usr/bin/env perl

#graehl@isi.edu

my @a=<>;
my $l=$#a;
my $t=$a[$l];$a[$l]=$a[0];$a[0]=$t;

print @a;
