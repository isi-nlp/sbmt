#!/usr/bin/env perl

use 5.006;
use warnings;
use strict;
use Memoize; 

my @deriv; 
my %id2rules;

sub read_deriv {
	# param 1 : subtree-root, which is a reference to a hash.
	# param 2 : tree tokens (including brackets '(' and ')').
	# param 3 : reference to the index of current token in
	#           tree tokens.
	my $root   = shift;
	my $tokens = shift;
	my $index  = shift;

	if($tokens->[$$index] eq "("){
		++$$index;
		$root->{LABEL} = $tokens->[$$index];
		++$$index;
		my @children ;
		while( ! ($tokens->[$$index] eq ")")){
			my $root1 = {};
			read_deriv($root1, $tokens, $index);
			push @children, $root1;
			++$$index;
		}
		undef $root->{CHILDREN};
	    push  @{$root->{CHILDREN}}, @children;
	} else {
		$root->{LABEL} = $tokens->[$$index];
		@{$root->{CHILDREN}} = ();
	}
}

sub get_tree {
	# param 1 deriv tree root.
	# param 2 mapping from rule id to rule strings.
	# return the tree string
	my $deriv_root = shift;
	my $id2r       = shift;

	my $tree_string = "";

	#print $deriv_root->{LABEL} . " CC\n";
	if(!exists $deriv_root->{LABEL}){
		return;
	}
	my $r_string = $id2r->{$deriv_root->{LABEL}};

	my @segs;
	if( defined $r_string && $r_string =~ /(.*) ->(.*)/){
		my $lhs = $1;
		my $rhs = $2;

		$lhs =~ s/\("(\S+)"\)/\($1\)/g;
		$lhs =~ s/([^ \(\)]+)\(/\($1 /g;
		$lhs =~ s/\)(?=[^ ])/\) /g;
		#print $lhs . "\n";

		my @vars;
		$rhs .= " ";
		while( $rhs =~ / x(\d+)(?= )/g){ push @vars, $1; }

		#
		# Re-order the children so that they are in english
		# order.
		#
		if(scalar(@vars) != scalar(@{$deriv_root->{CHILDREN}})){
			my $i = scalar @vars;
			my $j =  scalar @{$deriv_root->{CHILDREN}};
			die "Mismatch vars in rule $r_string: #var $i, #child $j: !\n";
		}
		my @children;
		for(my $i = 0; $i < scalar(@vars); ++$i){
			$children[$vars[$i]] = $deriv_root->{CHILDREN}[$i];
		}
		undef $deriv_root->{CHILDREN};
		push @{$deriv_root->{CHILDREN}}, @children;

		$lhs =~ s/\b(x\d+:[^ \)]+)/  __XX__ $1 __XX__  /g;

	    @segs =  split(/__XX__/, $lhs);
		my $var_index = 0;
		foreach my $seg (@segs) {
			$seg =~ s/^\s+//g;
			$seg =~ s/\s+/ /g;
			if($seg =~ /x\d+:\S+/) {
				$tree_string .= get_tree($deriv_root->{CHILDREN}[$var_index], $id2r);
				++$var_index;
			} else { 
				$tree_string .= $seg . " ";
			}
		}
	} else {
		die "$deriv_root->{LABEL} unseen!\n";
	}

	return $tree_string;
}

#memoize('read_deriv');
#memoize('get_tree');

my $deriv_tree = {};
while( <STDIN> ){
   if( /<deriv>(.*)<\/deriv>/ ){
	   my $deriv_string = $1;
	   $deriv_string =~ s/(\)|\()/ $1 /g;
	   $deriv_string =~ s/^\s+|\s+$//g;
	   $deriv_string =~ s/\s+/ /g;
	   @deriv = split /\s/, $deriv_string;
	   $deriv_tree = {};
	   my $index = 0;
	   read_deriv($deriv_tree, \@deriv, \$index);

   } elsif (/<XRS>/){
	   undef %id2rules;
   } elsif (/<\/XRS>/){
	   my $tree = get_tree($deriv_tree, \%id2rules);
	   $tree =~ s/(\(\S+)\s+(?=\()/$1~0~0 -0 /g;
	   print $tree . "\n";
	   undef %$deriv_tree;
   } elsif ( /(.* -> .*) ### .*\bid=(\d+)/ ){
	   $id2rules{$2} = $1;
   }
}



