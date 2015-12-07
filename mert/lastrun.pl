#!/usr/bin/perl

while (<>) {
    if (/^start 1-best/) {
	@sents = ();
	$flag = 1;
    } elsif (/^end 1-best/) {
	$flag = 0;
    } elsif ($flag) {
	push(@sents, $_);
    }
}

for $sent (@sents) {
    print $sent;
}
