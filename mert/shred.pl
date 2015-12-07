#!/usr/bin/perl

sub return_fh {             # make anon filehandle
    local *FH;              # must be local, not my
    open FH, ">$_[0]";
    return *FH;
}

@outfile = ();
foreach $outfilename (@ARGV) {
    push(@outfile, return_fh($outfilename));
}

$i=0;
while (<STDIN>) {
    print { $outfile[$i] } $_;
    $i = ($i+1) % @outfile;
}
