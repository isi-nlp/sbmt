#!/bin/sh
#! -*- perl -*-
eval 'exec perl -x $0 ${1+"$@"} ;'
 if 0;

#!/usr/bin/perl -w
use strict;
use File::Basename;
my $scriptdir;
my $scriptname;
BEGIN {
	$scriptdir = &File::Basename::dirname($0);
 	($scriptname) = &File::Basename::fileparse($0);
 	push @INC, $scriptdir;
}

require "model1_db.pl";

#--------------------------------------------------------------
# cmbgen_dir.pl
#   - to find probabilities with direct hash
# Author: Keonhoe Cha
# modified cma then graehl (no idea how this works)
# Date: 01/11/2002
# Modified: 01/12/2002
# Input: sent.test (Test sentences) 
# Output: Sentence bigram probabilities, i.e., *.trp
#-------------------------------------------------------------

scalar @ARGV >= 1 or &usage;

sub usage {
print $scriptname;
print <<'EOF';
 f-e.actual.t1.final outprefix
where f-e.actual.t1.final is a Model 1 probability file (produced by GIZA)
reads your text in tokenized format (from file or STDIN), one sentence per line and
prints the transition probability matrix, saving a copy to outprefix.trpl and also writing
outprefix.all_pair
the matrix file begins with a line giving N sentence lengths
it continues with NxN rows/columns (row = source, col = dest)
sentence 0 is the fictitious start/end "<TEXT>" sentence
EOF
exit -1;
}
my $prob_file = shift @ARGV;
my $outprefix=shift @ARGV;
my $allpairfile="$outprefix.all_pair";
my $trpfile="$outprefix.trpl";

my $smoothing_word_gen_prob = 2.5e-08;

&init_model1_db($prob_file);

open(ALL_PAIR,">$allpairfile");

my @sen_queue=<>; # read input
my @prob_matrix;
my @word_matrix;
&cal_prob;
&store_result;
close(ALL_PAIR);

&close_db;



sub store_result {
	open(TEST_TRP,">$trpfile") or die "Can't open file:$trpfile: $!\n";

	print "Sentence Transition Probabilities (sentence 0 = <TEXT>, row = source, column = succesor:\n";
	print "         ";

	my $text_len = scalar(@sen_queue)+1;
	for(my $i = 0; $i < $text_len; $i++) {
	    my $len=scalar(@{$word_matrix[$i]});
	    print "sentence $i($len)";
	    print TEST_TRP "$len "
	}
	print "\n";
	print TEST_TRP "\n";

	for(my $i = 0; $i < $text_len; $i++) {
		print "sentence $i  ";
		for(my $j = 0; $j < $text_len; $j++) {
			print $prob_matrix[$i][$j]." ";
			print TEST_TRP $prob_matrix[$i][$j]." ";
		}
		print "\n";
		print TEST_TRP "\n";
	}
	close(TEST_TRP) or die "Can't close file: $!\n";
}


sub cal_prob {
    my $NULL='NULL';
    my $TEXT="<TEXT>";
	my $no_sen = scalar(@sen_queue);
	@prob_matrix = ();
	@word_matrix = ();
	my $i;
#----- w/ P(t|NULL)----------------------
	push @word_matrix, [$NULL,$TEXT]; #cma text
	foreach (@sen_queue) {
	           chomp;
		push @word_matrix, [ $NULL, split ];
	}

	for($i=0; $i<=$no_sen;$i++) {
		$prob_matrix[$i][$i] = -1;
	}

#------------------------------------------------
# Prob. Calculation
#------------------------------------------------
	for($i=0; $i<=$no_sen; $i++) {
		my $source_len = scalar(@{$word_matrix[$i]});
		for(my $j=0; $j<=$no_sen; $j++) {
			if($i == $j) {
				next;
			}
#----------------------------------------------
# For Analysis
#----------------------------------------------
			print ALL_PAIR "sentence $i: ";
			print_word_array(@{$word_matrix[$i]});
			print ALL_PAIR "sentence $j: ";
			print_word_array(@{$word_matrix[$j]});
#------------------------------------------------

			my $target_len = scalar(@{$word_matrix[$j]});
			my $sen_trans_prob = 1.0;

			for(my $t=1; $t < $target_len; $t++) {

#------------------------------------------------------
# For exclusion of functional words
#-----------------------------------------------------
#next				if(function_word($word_matrix[$j][$t]));
#------------------------------------------------------
				my $word_gen_prob = 0.0;
				my $prob_sum = 0.0;
				for(my $s=0; $s < $source_len; $s++) {
				    #					$key = $word_matrix[$i][$s]."+++".$word_matrix[$j][$t];
				    #				$prior_prob = $prob_db{$key};
				    my $prior_prob = &get_prob($word_matrix[$i][$s],$word_matrix[$j][$t]);
				    $prior_prob = 0 unless defined($prior_prob);
				    print ALL_PAIR "      P($word_matrix[$j][$t]\|$word_matrix[$i][$s]):$prior_prob\n";
				    $prob_sum += $prior_prob;
				}
				# Uniform Distribution Assumed
				if($prob_sum > 0) {
					$word_gen_prob = $prob_sum/$source_len;
				}
				else {
#-------------------------------------------------
# Smoothing, Method 1
#------------------------------------------------
					$word_gen_prob = $smoothing_word_gen_prob;
#---- End of Method 1 ----------------------------
				}
				print ALL_PAIR "   Sentence $i -> $word_matrix[$j][$t]:$word_gen_prob\n";
				$sen_trans_prob *= $word_gen_prob;
			}
			print ALL_PAIR "P(Sentence $j \|Sentence $i):$sen_trans_prob\n";
			$prob_matrix[$i][$j] = $sen_trans_prob;
		}
	}
}

sub print_word_array {
    print ALL_PAIR join(' ',@_),"\n";
}

sub function_word {
	my @preposition = ("to","of","for","in","at","with","on","from","into","about","up","through");
	foreach my $prep (@preposition) {
		if($prep eq $_[0]) {
			return 1;
		}
	}
	return 0;
}
