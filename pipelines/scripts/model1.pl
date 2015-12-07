#!/usr/bin/env perl

# modified from /home/nlg-03/wang11/sbmt-bin/v3.0/rule-prep/model1-adder/v1.3/add-model1-to-xrs-rules.pl

# usage: model1.pl <featname> <ttable> [<tmpdir>]

# input:  x \t y \t id
# output: id \t featname=e^log(P(y|x))

# where P(y|x) is computed like Model 1 using the <ttable> which has the format
# ttable: x y P(y|x)

use strict;
use warnings;

my $featname = shift @ARGV or die "Please specify feature name";

my $model1inverse_file = shift @ARGV or die "Please specify ttable file";

# the alternative tmp dir.
my $my_tmp_dir = "";
$my_tmp_dir = shift @ARGV or $my_tmp_dir = "/tmp";
print STDERR "using temp dir: $my_tmp_dir\n";

# create tmp dir in case it is not there.
`mkdir -p $my_tmp_dir`;

my $penalty=20;			# penalty for events not there
my $done = 0;
while (!$done) {
    my $rule_lines_file = `mktemp $my_tmp_dir/add-model1.XXXXXX`;
    chomp($rule_lines_file);
    open(IN_COPY, ">$rule_lines_file") or die "Couldn't open $rule_lines_file";

    my %ttable;
    
    # first step, read in vocab
    
    while (keys(%ttable) < 10000000) { 
	$_ = <>;
	if (!$_) {
	    $done = 1;
	    last;
	}
	my @fields = split('\t');
	my @lhsWords = split(' ', $fields[0]);
	my @rhsWords = split(' ', $fields[1]);
	my $id = $fields[2];
	my $original_line = $_;	# the whole line
	print IN_COPY $original_line;
	foreach my $e (@lhsWords, "NULL") { 
	    foreach my $f (@rhsWords) { 
		$ttable{"$e $f"} = undef;
	    }
	}
    }
    close(IN_COPY);
    
    print STDERR "Need to retrieve " . scalar(keys %ttable) . " ttable entries\n";
    
    die "ERROR: cannot read $model1inverse_file\n" unless -r $model1inverse_file;
    # first treat it as bz2 file
    my $ret = system("bunzip2 -t $model1inverse_file");
    # if the exit code is !0, then it is not bunzip2 file.
    if($ret){
	warn "INFO: treating $model1inverse_file as normal file\n";
	open(MODEL1, "<$model1inverse_file") or die "Couldn't open $model1inverse_file:$!";
    } else {
	warn "INFO: treating $model1inverse_file as bz2 file\n";
	open(MODEL1, "bunzip2 -c $model1inverse_file|") or die "Couldn't open $model1inverse_file:$!";
    }
    
    print STDERR "Reading model1 scores\n";
    
    # then get scores which apply to our vocab
    
    while (<MODEL1>) { 
	chomp;
	my @fields = split;
	if (@fields != 3) { warn "ignored line in ttable: $_\n"; next; }
	my ($e, $f, $prob) = @fields;
	if (exists($ttable{"$e $f"})) {
	    $ttable{"$e $f"} = -log($prob);
	} 
    }
    
    close(MODEL1);

    print STDERR "Model 1 scores read\n";
    
    # go back over rules
    
    open(IN_COPY, "$rule_lines_file") or die "Couldn't open $rule_lines_file";
    
    while (my $line = <IN_COPY>) { 
	my @fields = split('\t', $line);
	my @lhsWords = split(' ', $fields[0]);
	my @rhsWords = split(' ', $fields[1]);
	my $id = $fields[2];
	
	my $events=0;
	my $total_log=0;
	
	# for each foreign word, find most likely english generating word.
	# the real Model 1 uses sum, not max
	foreach my $f (@rhsWords) { 
	    my $min_cost=100000000;
	    foreach my $e ((@lhsWords, "NULL")) { # check all english words
		my $curr_cost=0;
		if (!defined($ttable{"$e $f"})) { 
		    $curr_cost= $penalty;
		} else { 
		    $curr_cost= $ttable{"$e $f"};
		}
		if ($curr_cost<$min_cost) { 
		    $min_cost = $curr_cost;
		}
	    }				# end for each english
	    $total_log+=$min_cost;
	    
	}
	if ($total_log==0) { 
	    print "$id\t$featname=e^0 no$featname=1\n";
	} else { 
	    printf "$id\t$featname=e^%.6f\n", -$total_log;
	}
    }
    
    close(IN_COPY);
    
    # delete the temp file.
    if( -e "$rule_lines_file") {
	`rm -f $rule_lines_file`;
    }
}
    
