#!/usr/bin/perl
# make rules from lexicon and mapping
use FindBin;
use lib $FindBin::Bin;
use green;
use strict;
my ($efile, $ffile, $mapfile, $invmapfile, $outfile, $tagfile) = @ARGV;
my %emap = ();
my %fmap = ();
open IN, $efile or die "Can't open $efile: $!\n";
my $ecounter = 0;
my $demotag = &green::getDefaultTag("the");
while (<IN>) {
    /^(\d+)\s+(\S+)\s+(\d+)$/ or die "Bad format: $_\n";
    my $word = $2;
    $word =~ s/\"/\'\'/g;
    $emap{$1}{WORD} = $word;
    $emap{$1}{COUNT} = $3;
    $ecounter++;
}
close IN;

print "Read $ecounter from $efile\n";

my $fcounter = 0;
open IN, $ffile or die "Can't open $ffile: $!\n";
open OUT, ">$outfile" or die "can't open $outfile: $!\n";
while (<IN>) {
    /^(\d+)\s+(\S+)\s+(\d+)$/ or die "Bad format: $_\n";
    $fmap{$1}{WORD} = $2;
    $fmap{$1}{COUNT} = $3;
    $fcounter++;
}
close IN;

print "Read $fcounter from $ffile\n";

my %taglist = ();
my %tagmap = ();
if ($tagfile) {
    open IN, $tagfile or die "Can't open $tagfile: $!\n";
    while (<IN>) {
	next if (/^\(.*\)\s+$/);
	/(\S+)[\s]+([^\|\s]+)/ or die "Bad format: $_\n";
	my $word = $1;
	$word =~ s/\"/\'\'/g;
	my $tag = $2;
	if ($tag eq "\#") {
	    print "$word $tag\n";
	    $tag = "SYM";
	}
	if ($tag eq "") {
	    print "$word [$tag]\n";
	    $tag = "''";
	}
	if ($tag eq "1991)") {
	    print "$word $tag\n";
	    die;
	}
	$taglist{$tag} = 1;
	$word =~ tr/A-Z/a-z/;
	$tagmap{$word} = $tag;
    }
    close IN;
    print "Tag for \"the\" is ".$tagmap{"the"}."\n";
}

print "Tags:\n";
foreach my $tag (keys %taglist) {
    print "\t$tag\n";
}
my %invmap = ();
open IN, "$invmapfile" or die "Can't open $invmapfile: $!\n";
while (<IN>) {
    /^(\d+)\s+(\d+)\s+([\d.\-e]+)$/ or die "Bad format: $_\n";
    next if ($1 eq "0" or $2 eq "0");
    $invmap{$2}{$1} = $3;
}
close IN;

open IN, "$mapfile" or die "Can't open $mapfile: $!\n";
my $nextid = -10000;
my $outcount = 0;
my $noinversecount = 0;
while (<IN>) {
    /^(\d+)\s+(\d+)\s+([\d.\-e]+)$/ or die "Bad format: $_\n";
    next if ($1 eq "0" or $2 eq "0");

    my $eword = $emap{$1}{WORD};
    my $fword = $fmap{$2}{WORD};
    if ($fword eq "") {
	print "Error: No foreign word with value $2!\n";
	next;
    }
    my $val = log ($3);
    my $invscore = 0;
    if ($invmap{$1}{$2}) {
	$invscore = log($invmap{$1}{$2});
    }
    else {
	$noinversecount++;
    }

    my $tag = $tagmap{$eword};
    # if tag couldn't be found, use a default;
    $tag = &green::getDefaultTag($eword) unless ($tag);
    my $rule = "$tag(\"$eword\") -> \"$fword\" \#\#\# id=$nextid lexflag=1 lexicon=e^$val invlexicon=e^$invscore\n";
    print OUT $rule;
    $nextid--;
    $outcount++;
    if ($outcount % 100000 == 0) {
	print "$outcount\n";
    }
}    
print "$noinversecount rules had no inverse\n";
print "Printed $outcount rules\n";
close IN;
close OUT;


    
	
