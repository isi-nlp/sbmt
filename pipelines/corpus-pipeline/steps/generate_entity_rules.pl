#!/usr/bin/env perl
# Input: 1) dirpath/infile: a file of chinese text marked up by the entity finder. 
#           one sentence per line. lines assumed to be headed and tailed by <seg>, </seg>           

#        2) outdir: a directory in which output files will be placed
#        3) data: hashable genericized and categorized rules

# Output: 1....n) vocab files, of the form outdir/infile.voc/infile.voc.i where i is the 
#            line number (sentence number) of infile. All the english words in all the 
#            entities detected. repetitions assumed. multiple words per line.
#         n+1....2n) rules files of the form outdir/infile.entrules/infile.entrules.i, 
#            where i is the line number (sentence number) of infile.

use FindBin;
use lib $FindBin::Bin;
use green;
use strict;
my $topstr = 1;
my $datafile;
my $currsent = 0;
my $db = createRuleHash($datafile);

# stuff to make this seem like real rules
# start counting ids at -2 and proceed DOWN
my $nextid = -2;
my $rootprobit1 = "e^0";
my $unitrootprobit1 = "e^0";
my $greenval = "1";

while (<>) {
    # make sure it's a sentence
    next unless /<seg>/;
    $currsent++;
    my $line = $_;
    # find the pairs of translation, type.
    while ($line =~ /(.*?)(<NEMATCH[^<>]+>)([^<>]+)<\/NEMATCH>(.*)/) {
	$line = $4;
	my $cside = $3;
	my $tag = $2;
 	$cside =~ s/^\s+//;
 	$cside =~ s/\s+$//;
	my ($engs, $feats) = ($tag =~ /english=\"([^\"]+)\".*?FEATURES=\"([^\"]*)\"/);
	next unless ($engs);
	my @engarr = split /\s+\|\s+/, $engs;
	# coarsely add the english to the vocab file
	print VOC (join " ", @engarr)."\n";
	my @featarr = split /\s+\|\s+/, $feats;
	# send off each pair to the engine
	# if topstr is set, only use the top 2 pairs
	my $counter = 0;
	while (@engarr) {

	    if ($counter > 2 && $topstr) {
		last;
	    }
	    my $eside = shift @engarr;
	    my $feat = "";
	    if (@featarr) {
		$feat = shift @featarr;
	    }
	    my @rules = &green::getRulesFromEnt($eside, $cside, $feat, 1, $db);
	    foreach my $rule (@rules) {
		$counter++;
		print "$rule ### id=$nextid green=$greenval root_prob_it1=$rootprobit1 unit_root_prob_it1=$unitrootprobit1 sid=$currsent\n";
		    $nextid--;
	    }
	}
	# less strict version if nothing added
	if ($counter == 0) {
	    while (@engarr) {
		
		if ($counter > 2 && $topstr) {
		    last;
		}
		my $eside = shift @engarr;
		my $feat = "";
		if (@featarr) {
		    $feat = shift @featarr;
		}
		my @rules = &green::getRulesFromEnt($eside, $cside, $feat, 0, $db);
		foreach my $rule (@rules) {
		    $counter++;
		    print "$rule ### id=$nextid green=$greenval root_prob_it1=$rootprobit1 unit_root_prob_it1=$unitrootprobit1 sid=$currsent\n";
			$nextid--;
		}
	    }
	}
    }
}
	    
