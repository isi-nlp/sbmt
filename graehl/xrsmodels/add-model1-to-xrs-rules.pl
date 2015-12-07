#!/usr/bin/perl

use strict;
use warnings;

my $model1inverse_file = shift @ARGV or die "Please specify model 1 inverse file";


my $rule_lines_file = `mktemp /tmp/add-model1.XXXXXX`;
chomp($rule_lines_file);
open(IN_COPY, ">$rule_lines_file") or die "Couldn't open $rule_lines_file";


my %english_vocab;
my %foreign_vocab;

$english_vocab{"NULL"} = 1;

my $penalty=20;			# penalty for events not there

# first step, read in vocab

while (<>) { 
  if (/^(([^( ]+)\(.*\)) -> (.*) ### (.*)/) { # for each rule line
    my $lhs = $1;		# lhs of rule
    my $nt = $2;		# non terminal at top
    my $rhs = $3;		# rhs of rule
    my $attributes = $4;	# attribute strings
    my $original_line = $_;	# the whole line

    my @rhsArr = split(' ', $rhs); # rhs tokens 

    my @rhsWords;

    foreach my $rhsToken (@rhsArr) { # for each token
      if ($rhsToken =~ /^x/) { 	# if its an xrs variable
      } else { 			# lexical item
	$rhsToken =~ s/^\"//;
	$rhsToken =~ s/\"$//;
#	print STDERR "Saving F: $rhsToken\n";
	push(@rhsWords, $rhsToken);
      }
    }
    
    my @lhsWords;
    while ($lhs =~ /(\"(.+?)\")/g) { 
      my $match = $2;
#      print STDERR "Saving E: $match\n";
      push (@lhsWords, $match);
      
    }
    print IN_COPY $original_line;
    foreach my $e (@lhsWords) { 
      $english_vocab{$e}=1;
    }
    foreach my $f (@rhsWords) { 
      $foreign_vocab{$f}=1;
    }
  } else { 
    print "$_";			# if on the input is a comment, we pass it through.
    print STDERR "Ignored line: $_\n";
  }
}



open(MODEL1, "bunzip2 -c $model1inverse_file |") or die "Couldn't open $model1inverse_file";

my %logscores;

print STDERR "Reading model1 scores\n";

# then get scores which apply to our vocab

while (<MODEL1>) { 
  if (/(\S+) (\S+) (\S+)/) { 
    my $e = $1;
    my $f = $2;
    my $prob = $3;
    if (defined($english_vocab{$e}) and defined($foreign_vocab{$f})) { 
      $logscores{$1." ### ".$2} = -log($prob);
    } 
  } else { 
    warn "Ignored line: $_";
  }

}

close(MODEL1);
close(IN_COPY);
print STDERR "Model 1 scores read\n";

# go back over rules

open(IN_COPY, "$rule_lines_file") or die "Couldn't open $rule_lines_file";

while (my $line = <IN_COPY>) { 
  if ($line =~ /^(([^( ]+)\(.*\)) -> (.*) ### (.*)/o) { # for each rule line
    my $lhs = $1;		# lhs of rule
    my $nt = $2;		# non terminal at top
    my $rhs = $3;		# rhs of rule
    my $attributes = $4;	# attribute strings
    my $original_line = $line;	# the whole line


    my @rhsArr = split(' ', $rhs); # rhs tokens 

    my @rhsWords;

    foreach my $rhsToken (@rhsArr) { # for each token
      if ($rhsToken =~ /^x/) { 	# if its an xrs variable
      } else { 			# lexical item
	$rhsToken =~ s/^\"//;
	$rhsToken =~ s/\"$//;
	push(@rhsWords, $rhsToken);
      }
    }
    
    my @lhsWords;
    while ($lhs =~ /(\"(.+?)\")/g) { 
      my $match = $2;
      push (@lhsWords, $match);
      
    }
    my $events=0;
    my $total_log=0;
    foreach my $f (@rhsWords) { # for each foreign word, find most likely english generating word
      my $min_cost=100000000;
      foreach my $e ((@lhsWords, "NULL")) { # check all english words
	my $curr_cost=0;
	if (!defined($logscores{$e." ### ".$f})) { 
	  $curr_cost= $penalty;
	} else { 
	  $curr_cost= $logscores{$e." ### ".$f};
	}
	if ($curr_cost<$min_cost) { 
	  $min_cost = $curr_cost;
	}
      }				# end for each english
      $total_log+=$min_cost;
    }
    chomp($original_line);
    if ($total_log==0) { 
      print "$original_line model1inv=1\n";
    } else { 
      print "$original_line model1inv=e^-".$total_log."\n";      
    }
    
  } else { 
    print STDERR "Ignored line: $line\n";
  }
}

close(IN_COPY);
