#
# Provides standardized parsing for source, target and feature tokens.
#
# Author: Jens-S. Vöckler voeckler at isi dot edu
# $Id$
#
package NLPRules;
use 5.006;
use strict;

require Exporter;
our @ISA = qw(Exporter);

# declarations of methods here. The commented body is to unconfuse emacs
sub count_words(@);		# { }
sub target_token($);		# { }
sub target_wordvar($);		# { }
sub target_variables($);	# { }
sub target_variable_pos($);	# { }
sub target_words($);		# { }
sub source_token($);		# { }
sub source_words($);		# { }
sub source_variables($);	# { }
sub signature(@);		# { }
sub feature_value($);		# { }
sub feature_spec($);		# { }
sub full_feature_spec($);	# { }
sub extract_lrf($);		# { }
sub extract_from_rule($;$$$);	# { }

use constant FOREIGN => '<foreign-sentence>'; # fs marker on TOP

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
# The tags allows declaration       use Tools ':all';
our @EXPORT_OK = qw(target_token target_wordvar target_variables 
		    target_variable_pos target_words
		    source_token source_words source_variables signature 
		    FOREIGN
		    feature_value full_feature_spec feature_spec 
		    count_words extract_lrf extract_from_rule);
our %EXPORT_TAGS = ( 'all' => [ @EXPORT_OK ]
		   , 'lhs' => [qw(target_token target_words target_variables target_variable_pos)] 
		   , 'rhs' => [qw(source_token source_words source_variables signature FOREIGN)]
		   );
our @EXPORT = qw();

our $VERSION = '1.1';		# last update: 2008-09-15 (jsv)
$VERSION = $1 if ( '$Revision$' =~ /Revision:\s+([0-9.]+)/ );

# Preloaded methods go here.

sub count_words(@) {
    # purpose: given an array with words, count duplicates
    # paramtr: @words (IN): array with words
    # returns: hash with each unique word as key, and occurances value
    #
    my %result = map { $_ => 0 } @_; # init hash from array
    foreach my $word ( @_ ) {
	++$result{$word};	# will not create warnings due to init
    }
    %result;
}

sub target_token($) {
    # purpose: given a LHS, return list of token 
    # warning: This is an expensive function, use sparingly
    # paramtr: $lhs (IN): Operator string representation of tree
    # returns: possibly empty list of token
    #
    my $lhs = shift; 

    my @result = (); 
    foreach ( split( ' ', $lhs ) ) {
	if ( s/\((\".+\")\)+$// ) {
	    my $terminal = $1;
	    s/\(+/ /g; 
	    push( @result, (split), $terminal ); 
	} elsif ( /(x\d+:[^()"" ]+)\)*$/ ) {
	    push( @result, $1 ); 
	} else {
	    # unknown
	    warn( __PACKAGE__, " Warning: Unparsed token $_\n" ); 
	}
    }
    @result;
}

sub target_wordvar($) {
    # purpose: given a LHS, return list of words or variables
    # warning: this is only slightly less expensive than target_token
    # paramtr: $lhs (IN): Operator string representation of tree
    # returns: possibly empty list of token
    #
    my $lhs = shift; 

    my @result = (); 
    foreach ( split( ' ', $lhs ) ) {
	if ( m/\((\".+\")\)+$/ ) {
	    push( @result, $1 ); 
	} elsif ( /(x\d+):[^()"" ]+\)*$/ ) {
	    push( @result, $1 ); 
	} else {
	    # unknown
	    warn( __PACKAGE__, " Warning: Unparsed token $_\n" ); 
	}
    }
    @result;
}

sub target_variables($) {
    # purpose: given a lhs, extract target language variables
    # paramtr: $lhs (IN): string with left-hand side of rule
    # warning: only slightly expensive
    # returns: array, possibly empty, with target variable indices
    #
    my $lhs = shift; 

    my @result = (); 
    foreach ( split( ' ', $lhs ) ) {
	push( @result, $1+0 ) if /x(\d+):[^()"" ]+\)*$/; 
    }
    @result;
}

sub target_variable_pos($) {
    # purpose: given a lhs, extract bound NTs into variable index
    # paramtr: $lhs (IN): string with left-hand side of rule
    # warning: only slightly expensive
    # warning: assumes that no variables occurs twice (last wins)!
    # returns: array, possibly empty, with NT at variable's slot
    #          if there are missing variables, slots may be undef
    #
    my $lhs = shift; 

    my @result = (); 
    foreach ( split( ' ', $lhs ) ) {
	$result[$1+0] = $2 if /x(\d+):([^()"" ]+)\)*$/; 
    }
    @result;
}

sub target_words($) {
    # purpose: given a lhs, extract target language words
    # paramtr: $lhs (IN): string with left-hand side of rule
    # returns: array, possibly empty, with target words
    #
    my $lhs = shift;
    my @result = ();

    # you must match parentheses and quotes outside string literal
    push( @result, $1 ) while $lhs =~ m{\(\"(.+?)\"\)+(?:\s|$)}g;

    grep( ! /FOREIGN/, @result );
}

sub source_token($) {
    # purpose: given a rhs, create a list of tokens
    # paramtr: $rhs (IN): string with right-hand side of rule
    # warning: RHS strings *must not* have whitespace in them
    # returns: array, possibly empty, with raw source token
    #
    split( ' ', shift() );
}

sub source_words($) {
    # purpose: given a rhs, extract source language words
    # paramtr: $rhs (IN): string with right-hand side of rule
    # warning: RHS strings *must not* have whitespace in them
    # returns: array, possibly empty, with source words
    #
    my $rhs = shift;
    my @result = ();

    foreach my $token ( source_token($rhs) ) {
	# remove quotes around string literal
	push( @result, substr($token,1,-1) ) if substr($token,0,1) eq '"';
    }

    grep( ! /FOREIGN/, @result );
}

sub source_variables($) {
    # purpose: given a rhs, extract source variables on the RHS
    # paramtr: $rhs (IN): string with right-hand side of rule
    # warning: RHS strings *must not* have whitespace in them
    # returns: array, possibly empty, with source var indices, in order
    #
    my $rhs = shift;
    my @result = ();

    foreach my $token ( source_token($rhs) ) {
	# make result numerical
	push( @result, substr($token,1)+0 ) if substr($token,0,1) eq 'x';
    }

    @result
}

sub signature(@) {
    # purpose: Collapses consequetive non-terminals.
    # paramtr: @_ (IN): array of RHS tokens, as from source_token
    # returns: signature of rule as string
    #
    my @result = ();
    my $flag = 0;               # seen x before, if set
    my $i = 0;
    foreach my $token ( @_ ) {
        if ( substr($token,0,1) eq 'x' ) {
            push( @result, 'x' . $i++ ) if ! $flag;
            $flag = 1;
        } else {
            push( @result, $token );
            $flag = 0;
        }
    }

    @result = ( 'x0' ) if @result == 0;
    join(' ',@result);
}

sub feature_value($) {
    # purpose: unconvert e^ and 10^ values
    # paramtr: $s (IN): input feature value
    # returns: possibly converted feature value, or as-is input
    #
    my $s = shift;
    if ( substr($s,0,2) eq 'e^' ) {
        exp( substr($s,2) );
    } elsif ( substr($s,0,3) eq '10^' ) {
        10 ** substr($s,3);
    } else {
        $s;
    }
}

sub feature_spec($) {
    # purpose: given a feature part, extract mapping key to value
    # paramtr: $feat (IN): string with feature portion of rule
    # returns: hash, possibly empty, with key to value mappings
    #
    my $feat = shift;
    my %result = ();

    while ( $feat =~ m{([^ =]+)=(\{\{\{(.*?)\}\}\}|(\S+))}g ) {
	$result{$1} = substr($2,0,1) eq '{' ? $3 : feature_value($4); 
    }

    %result;
}

sub full_feature_spec($) {
    # purpose: given a feature part, extract mapping key to value
    # paramtr: $feat (IN): string with feature portion of rule
    # returns: hash, possibly empty, with key to complex type mappings
    #          [0] parsed value 
    #          [1] original value string, including triple braces
    #          [2] predicate that is 1 for number values, 0 for strings
    #
    my $feat = shift;
    my %result = ();

    while ( $feat =~ m{([^ =]+)=(\{\{\{(.*?)\}\}\}|(\S+))}g ) {
	if ( substr($2,0,1) eq '{' ) {
	    $result{$1} = [ $3, $2, 0 ];
	} else {
	    $result{$1} = [ feature_value($4), $2, 1 ];
	}
    }

    %result;
}

sub extract_lrf($) {
    # purpose: extract LHS, RHS and FEATS from XRS rule.
    # paramtr: $rule (IN): XRS rule, possibly with trailing LF
    # returns: array with 0 (rules doe not have ' -> '), or 3:
    #          [0]: LHS
    #          [1]: RHS
    #          [2]: FEAT (may be empty string)
    #
    my $xrs = shift;

    if ( $xrs =~ /^\s*(.+?)\s->\s(.+?)(?:\s\#\#\#\s(.*))?\s*$/ ) {
	( $1, $2, $3 );
    } else {
	();
    }
}

sub extract_from_rule($;$$$) {
    # purpose: determine target language words in LHS
    # paramtr: $rule (IN): XRS rule, possibly with trailing LF
    #          $lref (IO): target words (aref) or undef
    #          $rref (IO): source words (aref) or undef
    #          $fref (IO): feature key to value (href) or undef
    # returns: 1 if the rule matched basic criterions (has ' -> ')
    #          0 if the rule was critically malformed (no ' -> ')
    #
    my @xrs = extract_lrf( shift() );

    if ( @xrs == 3 ) {
	@{$_[0]} = target_words($xrs[0]) if defined $_[0];
	@{$_[1]} = source_words($xrs[1]) if defined $_[1];
	%{$_[2]} = feature_spec($xrs[2]) if defined $_[2];

	1; 
    } else {
	0;
    }
}

1;

__END__

=head1 NAME

NLPRules - Some useful tools when parsing XRS rules in Perl.

=head1 SYNOPSIS

    use NLPRules qw(:all);

    my ($s,@twords,@swords,%feats);
    $s = extract_from_rule($xrs,\@twords,\@swords,\%feats);
    ...
    extract_from_rule( $xrs, \@twords );

=head1 DESCRIPTION

This module provides a basic set of tools to parse XRS rules in Perl.
The parsing deals pretty well with strings that have three quotes. 

=head1 FUNCTIONS

=over 4

=item [%wc = ] count_words(@words); 

This method counts the occurances of each word in the list of words, and
returns an array indexed by the unique word and their count as value.

=item [@tl = ] target_token($lhs);

This method splits the LHS of a rule, usually a tree in operator
notation, into a list of tokens. The resulting list has elements that
are one of plain non-terminal, variable-colon-non-terminal, or a quoted
word.

B<Note:> This method is computationally expensive, use sparingly. 

B<Note:> This method may print warnings on I<stderr>. 

=item [@wvl = ] target_wordvar($lhs);

This method splits the LHS of a rule into a list of either quoted
terminals or variables without non-terminal.

B<Note:> This method is computationally slightly less expensive than the
C<target_token> method.

B<Note:> This method may print warnings on I<stderr>. 

=item [@vl = ] target_variables($lhs);

This method extracts all target variables from the LHS of any valid XRS
rule. Note that this function does not take a full rule, just the LHS.
The result may be an empty array in the absence of any variables.
Otherwise, it is an ordered list of variable indices.

B<Note:> This method is computationally slightly less expensive than
both, the C<target_token> and C<target_wordvar> methods. 

=item [@ntl = ] target_variable_pos($lhs);

This method extracts the non-terminals bound to target variables from
the LHS of any valid XRS rule. Note that this function does not take a
full rule, just the LHS. The result may be an empty array in the absence
of any variables. Otherwise, the bound non-terminal occupies the slot of
its variable index. 

In case of missing variables, the resulting array may contain C<undef>
values. In case of duplicate variables (this should not happen, but is
not checked for), the right-most duplicate wins.

B<Note:> This method is computationally slightly less expensive than
both, the C<target_token> and C<target_wordvar> methods. 

=item [@wl = ] target_words($lhs);

This method extracts all target language words (usually English) from
the given left-hand side of any XRS rule. Note that this function does
not take a full rule, just the LHS. The result may be an empty array.

If you are only interested to B<only> extract target language words from
rules in a speedy manner, you should invoke C<target_words> directly
from the following looped expression:

    while ( ... ) {
        if ( /^\s*(.+?) -> / ) {
            my @target = target_words($1);

If you require I<unique> words, try 

            my @uniq = keys %{{ map { $_ => 1 } target_words($1) }};

If you need to count the unique words, try

            my %uniq = count_words( target_words($1) );

The C<target_words> method is optimized for word extraction from a LHS. It
does B<not> employ the C<target_token> method.

=item [@tl = ] source_token($rhs);

This method separates a given RHS into a list of token. Mind that these 
token are raw: Variables are still starting with an B<x> and words are
still quoted. 

The result of C<source_token> is suitable to compute a B<signature> from.

=item [@wl = ] source_words($rhs);

This method extracts all source language words (usually French) from the
given right-hand side of any XRS rule. Note that this function does not
take a full rule, just the RHS. It uses C<source_token> to extract the
list of tokens from the RHS. The result may be an empty array. 

If you are only interested to B<only> extract source language words from
rules in a speedy manner, you should invoke C<source_words> directly
from the following looped expression:

    while ( ... ) {
        if ( / -> (.+?)(?: \#\#\# |\s*$)/ ) {
            my @source = source_words($1);

If B<all> your rules B<always> have features, the regular expression in
the C<if> statement simplifies to:

        if ( / -> (.+?) \#\#\# / ) {

=item [@vl = ] source_variables($rhs);

This method extract all source variables from the RHS of any valid XRS
rule. Note that this function does not take a full rule, just the RHS.
The result may be an empty array in the absence of any variables.
Otherwise, it is an ordered list of variable indices.

It uses C<source_token> to extract the list of tokens from the RHS.

For some thoughts on short-cuts, see I<source_words>. 

If you want to transform the list into an array that has defined values
only at positions where variables exist, you can transform the result
with the following expression:

    my @vars = ();
    foreach my $index ( source_variables($rhs) ) {
         $vars[$index] += 1; # will blah if use warnings
    }

After this transformation, array C<@vars> has defined entries only
where variables on the source side exist.

=item $s = signature( @token );

The C<signature> function takes a list of right-hand side (RHS) tokens,
which are either variables starting with prefix C<x>, or terminal tokens
starting with a double quote C<E<quot>>. The list is then trimmed of
any variables on the left and right boundaries. Any consequetive
interior variables are collapsed into one variable. All new variables
are renumbered. The result is what is called the I<signature> of the RHS
of a grammar rule.

If the input is a RHS token list like:

    x0 "A" x1 x2 "B" x3

then the signature function would generate a string like:

    "A" x0 "B"

The C<signature> function can be used to distinguish between lexical and
non-lexical rules: Non-lexical rules generate the lone output C<x0>.

C<signature> uses the output from C<source_token>. 

=item $v = feature_value($fvs); 

This method converts the value portion of a given feature into a number.
It is most useful with numerical values, and converts the exponent
syntax into Perl numbers, but won't harm strings.

=item [%kv = ] feature_spec($fvs);

This method attempts to store the feature specification into a Perl
hash. Given the feature string portion of an XRS rule, it parses
entities by breaking them on their first equal sign. The result may be
an empty hash.

If you are only interested to B<only> extract the feature map from rules
in a speedy manner, you should invoke C<feature_spec> directly from the
following looped expression:

    while ( ... ) {
        if ( / \#\#\# (.+)\s*$/ ) {
            my %feats = feature_spec($1);

or, even faster, if you don't mind new-lines: 

    while ( ... ) {
        if ( ($p = index($_," ### ")) > 0 ) {
            my %feats = feature_spec(substr($_,$p+5));

Note: You need to make special arrangements, if you can have rules that
don't have any feature specification on them, i.e. above expression
won't match.

=item [%kv = ] full_feature_spec($fvs);

This method preserves more of the original input data when storing the
feature specification into a Perl hash. Given the feature string portion
of an XRS rule, it parses entities by breaking them on their first equal
sign. The result may be an empty hash.

Each key in the result will map to an array with exactly 3 values: 

=over

=item [0] parsed value

=item [1] original value from rule, including braces and roofs

=item [2] type predicate to indicate a number (1) or string (0). 

=back

If you are only interested to B<only> extract the feature map from rules
in a speedy manner, you should invoke C<feature_spec> directly from the
following looped expression:

    while ( ... ) {
        if ( / \#\#\# (.+)\s*$/ ) {
            my %feats = feature_spec($1);

Note: You need to make special arrangements, if you can have rules that
don't have any feature specification on them, i.e. above expression
won't match.

=item @xrs = extract_lrf( $rule );

This method extracts the three components, LHS, RHS and FEATS, as
strings from the rule. Trailing line separators in the rule are not a
problem, and do not need to be removed. The components are returned in
that order: LHS, RHS, FEATS.

In case of a bad rule, the returned array is empty. Please note that the
third item may be an empty string, if the rule does not have features.

=item [$ok = ] extract_from_rule( $rule, \@twords, \@swords, \%feats );

This combinatory method parses a full rule into the words and features.
The first argument is the full XRS rule, which may still have a trailing
line separator attached to it (not a problem). 

Any of the next three arguments may be C<undef> to avoid parsing that
portion of the rule. The first argument takes an array reference to the
target language words from the LHS of the rule. The second argument
takes an array reference to the source language words from the RHS of
the rule. The final argument takes a hash reference to the feature
mappings.

This function works on rules with and without features attached to them.
Internally, the sides are extracted using the C<extract_lrf> function
above.

Usually, the sub will clean the array and hash references passed. If the
return value of the function is 0, the references are untouched. 

=back

=head1 AUTHOR

Jens-S. VE<ouml>ckler, C<voeckler at isi dot edu>

=head1 COPYRIGHT AND LICENSE

This file or a portion of this file is licensed under the terms of the
Globus Toolkit Public License, found in file GTPL, or at
http://www.globus.org/toolkit/download/license.html. This notice must
appear in redistributions of this file, with or without modification.

Redistributions of this Software, with or without modification, must
reproduce the GTPL in: (1) the Software, or (2) the Documentation or
some other similar material which is provided with the Software (if
any).

Copyright 2008 The University of Southern California. All rights
reserved.

=cut
