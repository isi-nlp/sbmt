#!/usr/bin/env perl
#      CREATED:  01/23/06 13:37:39 PST by Wei Wang
#      modified by David Chiang 2007 Nov 5
#      modified by JSV 2008-05-05
#==============================================================================

use strict;
use FindBin qw($Bin);
use lib $Bin;

require "libgraehl.pl";
use FeatureFuncs;

while ( <STDIN> ) {
    # for each rule line
    # FIXME: Use NLPRules to match and extract
    # FIXME: If not, the condition below is possibly slow due to heavy backtracking
    if ( /^([^( ]+\(.*\)) ->(.*) \#\#\# (.*)/ ) { 
	my %attribHash = ();             # attribute lookup
	$attribHash{"lhs"} = $1;    # fill in attribHash
	$attribHash{"rhs"} = $2;    # fill in attribHash
	my $attr = $3;
	while ( $attr =~ /(\S+)=({{{.*?}}}|\S+)/g ) { 
	    # get key={{{This is text}}} attribute or key=value attribute
	    $attribHash{$1} = $2;    # fill in attribHash
	} 

	print $attribHash{"id"};
	print "\t";

	# permutation.
	print &perm_feat(\%attribHash, "nonmonotone");

	# is lexicalized.
	print " " . &is_lexicalized(\%attribHash, "is_lexicalized");

	# conditional sbtm prob (rf)
	if ( defined $attribHash{"lhscount"} ) {
	    print " " . &cond_feat(\%attribHash, "count", "lhscount", "trivial_cond_prob");
        }

	# moved missingWord into its own module
	print "\n";
    }
}



