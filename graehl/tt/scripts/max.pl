#!/usr/bin/env perl
while(<>) {
    while (m/([0-9]+)/g) {
	$max=$1 if $1>max;
    }
}
print "$max\n";
