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

sub target_tree($;$);		# { }
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

# use with care - read the doc & consider extract_lrf()
sub extract_lhs_safe($);	# { }
sub extract_lhs_fast($);	# { }
sub extract_rhs_safe($);	# { }
sub extract_rhs_fast($);	# { }
sub extract_feat_safe($);	# { }
sub extract_feat_fast($);	# { }

sub extract_lr_safe($); 	# { }
sub extract_lr_fast($); 	# { }

sub extract_lrf($);		# { }
sub extract_lrf_fast($);	# { }
sub extract_from_rule($;$$$);	# { }
sub extract_root_nt($);		# { }

use constant FOREIGN => '<foreign-sentence>'; # fs marker on TOP
use constant QUOTED_FOREIGN => '"<foreign-sentence>"'; # fs marker on TOP
				# 
use constant LHS_RHS_SEPARATOR => ' -> ';
use constant RHS_FEAT_SEPARATOR => ' ### ';
    ;
use constant LHS_RHS_LENGTH => length(LHS_RHS_SEPARATOR);
use constant RHS_FEAT_LENGTH => length(RHS_FEAT_SEPARATOR);

# you can import these variables explicitely or with qw(:re);
our $RE_LHS_FAST  = qr{^\s*(.*?) -> }o;
our $RE_LHS_SAFE  = qr{^\s*(.*?) -> }o;
our $RE_RHS_FAST  = qr{ -> (.+?) \#\#\# }o;
our $RE_RHS_SAFE  = qr{ -> (.+?)(?: \#\#\# |\s*$)}o;
our $RE_FEAT_SAFE = qr{ \#\#\# (.*)\s*$}o; 
our $RE_LR_FAST   = qr{^\s*(.*?) -> (.*?) \#\#\# }o; 
our $RE_LR_SAFE   = qr{^\s*(.*?) -> (.*?)(?: \#\#\# |\s*$)}o;
our $RE_LRF_FAST  = qr{^\s*(.*?) -> (.*?) \#\#\# (.*)}o;
our $RE_LRF_SAFE  = qr{^\s*(.*?) -> (.*?)(?: \#\#\# (.*))?\s*$}o;

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
# The tags allows declaration       use Tools ':all';
our @EXPORT_OK = qw(target_tree target_token target_wordvar 
		    target_variables target_variable_pos target_words
		    source_token source_words source_variables 
		    signature FOREIGN QUOTED_FOREIGN
		    LHS_RHS_SEPARATOR RHS_FEAT_SEPARATOR
		    LHS_RHS_LENGTH    RHS_FEAT_LENGTH
                    feature_value full_feature_spec feature_spec
		    $RE_LHS_SAFE $RE_LHS_FAST $RE_RHS_FAST $RE_RHS_SAFE
		    $RE_FEAT_SAFE $RE_LRF_SAFE $RE_LRF_FAST
		    $RE_LR_FAST $RE_LR_SAFE
		    extract_lhs_safe extract_rhs_fast extract_rhs_safe
		    extract_feat_safe extract_feat_fast
		    extract_lrf extract_lrf_fast extract_lr_fast
		    count_words extract_from_rule extract_root_nt);
our %EXPORT_TAGS = ( 'all' => [ @EXPORT_OK ]
		   , 're'  => [qw($RE_LHS_SAFE $RE_LHS_FAST 
				  $RE_RHS_FAST $RE_RHS_SAFE
				  $RE_FEAT_SAFE $RE_LRF_SAFE $RE_LRF_FAST
				  $RE_LR_FAST $RE_LR_SAFE)]
		   , 'lhs' => [qw(target_tree target_token target_words 
				  target_variables target_variable_pos
				  extract_lhs_safe extract_lhs_fast
				  extract_root_nt)] 
		   , 'rhs' => [qw(source_token source_words source_variables 
				  extract_rhs_fast extract_rhs_safe signature 
				  FOREIGN QUOTED_FOREIGN)]
		   );
our @EXPORT = qw();

our $VERSION = v1.4.1;		# last update: 2008-11-03 (special version for dc)
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

sub target_tree($;$) {
    # purpose: given a LHS, return a simple tree representation
    # warning: This is an expensive function
    # paramtr: $lhs (IN): Operator string representation of tree
    #          $pos (opt. IN): unified key to use instead of VAR/T/NT in nodes.
    # returns: undef or reference to a hash to represent the root node. Each node
    #          hash has the following layout: 
    #          KIDS => [] array reference to children nodes (inner nodes only). 
    #          $pos || NT => $  inner node with non-terminal's part-of-speech name tag
    #          $pos || T => $   leaf node with terminal string including quotes
    #          $pos || VAR => $ leaf node with variable including POS qualifier
    #          
    my $lhs = shift; 
    my $pos = shift; 

    my ($root,@stack);
    foreach ( split ' ', $lhs ) {
	while ( /(x\d+:[^()]+)|(\".*?\")(?=\)+(?:\s|$))|([^:()]+)\(|(\))/g ) {
	    # order if instructions by frequency, most likely first
	    # however, order RE with most complex longest matches first
	    if ( $3 ) {
		# non-terminal, diving down
		my $temp = { ($pos || 'NT') => $3, KIDS => [] };
		push( @{$root->{KIDS}}, $temp ) if defined $root; 
		push( @stack, $root ); 
		$root = $temp; 
	    } elsif ( $4 ) {
		# close paren, floating up
		die "unbalanced parentheses" unless @stack; 
		$root = pop(@stack) if defined $stack[$#stack];
	    } elsif ( $2 ) {
		# terminal leaf
		push( @{$root->{KIDS}}, { ( $pos || 'T' ) => $2 } );
	    } elsif ( $1 ) {
		# variable leaf
		push( @{$root->{KIDS}}, { ( $pos || 'VAR' ) => $1 } );
	    } else {
		die "this must not happen";
	    }
	}
    }
    
    $root;
}

sub target_token($) {
    # purpose: given a LHS, return list of token 
    # warning: This is an expensive function
    # paramtr: $lhs (IN): Operator string representation of tree
    # returns: possibly empty list of (NT, quoted strings, var-with-NT)
    #
    my $lhs = shift; 

    my (@result,$token) = (); 
    foreach ( split( ' ', $lhs ) ) {
	if ( s/\((\".+\")\)+$// ) {
	    $token = $1;
	    s/\(+/ /g; 
	    push( @result, (split), $token ); 
	} elsif ( s/(x\d+:[^()]+)\)*$// ) {
	    $token = $1;
	    s/\(+/ /g; 
	    push( @result, (split), $token ); 
	} else {
	    # unknown
	    warn( __PACKAGE__, " Warning: Unparsed token $_\n" ); 
	}
    }
    @result;
}

sub target_token_alt($) {
    # warning: Unfortunately, this is only marginally faster
    my $lhs = shift; 
    my @result = (); 
    push( @result, ( $1 || $2 ) )
	while ( $lhs =~ /(x\d+:[^ ()]+|\".*?\")\)*(?: |$)|([^ :()]+)\(/g ); 
    @result; 
}

sub target_wordvar($) {
    # purpose: given a LHS, return list of words or variables
    # warning: slightly less expensive than target_token
    # paramtr: $lhs (IN): Operator string representation of tree
    # returns: possibly empty list of (quoted string or variable)
    #
    my $lhs = shift; 

    my @result = (); 
    foreach ( split( ' ', $lhs ) ) {
	if ( m/\((\".+\")\)+$/ ) {
	    push( @result, $1 ); 
	} elsif ( /(x\d+):[^()]+\)*$/ ) {
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
	push( @result, $1+0 ) if /x(\d+):[^() ]+\)*$/; 
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
	$result[$1+0] = $2 if /x(\d+):([^() ]+)\)*$/; 
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

    grep { $_ ne FOREIGN } @result;
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

    grep { $_ ne FOREIGN } @result;
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

sub extract_lhs_fast($) {
    # purpose: extract LHS only from XRS rule.
    # paramtr: $rule (IN): XRS rule, possibly with trailing LF
    # returns: LHS string, possibly empty (which is Perl-false)
    #
    my $p = index( $_[0], LHS_RHS_SEPARATOR ); 
    $p > 0 ? substr($_[0],0,$p) : ''; 
}

sub extract_lhs_safe($) {
    # purpose: extract LHS only from XRS rule.
    # paramtr: $rule (IN): XRS rule, possibly with trailing LF
    # returns: LHS string, possibly empty (which is Perl-false)
    #
    ( shift() =~ /$RE_LHS_SAFE/o ) ? $1 : ''; 
}

sub extract_rhs_safe($) {
    # purpose: extract RHS only from XRS rule.
    # paramtr: $rule (IN): XRS rule, possibly with trailing LF
    # warning: This is rather slow matching
    # returns: RHS string, possibly empty (which is Perl-false)
    #
    ( shift() =~ /$RE_RHS_SAFE/o ) ? $1 : '';
}

sub extract_rhs_fast($) {
    # purpose: extract RHS only from 3-part XRS rule.
    # paramtr: $rule (IN): XRS rule with features
    # warning: Works only on rules WITH meta-data (features)
    # returns: RHS string, possibly empty (which is Perl-false)
    #
    # ( shift() =~ /$RE_RHS_FAST/o ) ? $1 : '';
    my ($p,$q);
    if ( ($p=index( $_[0], LHS_RHS_SEPARATOR )) > 0 ) {
	$p += LHS_RHS_LENGTH; 
	($q = index( $_[0], RHS_FEAT_SEPARATOR, $p )) == -1 ? 
	    substr( $_[0], $p ) : ### WARNING: Will include LF !!!
	    substr( $_[0], $p, $q-$p ); 
    } else {
	'';
    }
}

sub extract_feat_safe($) {
    # purpose: extract FEAT only from XRS rule.
    # paramtr: $rule (IN): XRS rule, possibly with trailing LF
    # returns: FEAT string, possibly empty (which is Perl-false)
    #
    ( shift() =~ /$RE_FEAT_SAFE/o ) ? $1 : ''; 
}

sub extract_feat_fast($) {
    # purpose: extract FEAT only from XRS rule.
    # paramtr: $rule (IN): XRS rule WITHOUT trailing LF
    # returns: FEAT string, possibly empty (which is Perl-false)
    #
    my $p;
    ($p = index($_[0],' ### ')) > 0 ? substr($_[0],$p+5) : ''; 
}

sub extract_lr_safe($) {
    # purpose: extract LHS and RHS from XRS rule.
    # paramtr: $rule (IN): XRS rule, possibly with trailing LF
    # returns: array with 0 (rules doe not have ' -> '), or 2:
    #          [0]: LHS
    #          [1]: RHS
    #
    ( ( shift() =~ /$RE_LR_SAFE/o ) ? ( $1, $2 ) : () ); 
}

sub extract_lr_fast($) {
    # purpose: extract LHS and RHS from XRS rule.
    # paramtr: $rule (IN): XRS rule, possibly with trailing LF
    # returns: array with 0 (rules doe not have ' -> '), or 2:
    #          [0]: LHS
    #          [1]: RHS
    #
    ( ( shift() =~ /$RE_LR_FAST/o ) ? ( $1, $2 ) : () ); 
}

sub extract_lrf($) {
    # purpose: extract LHS, RHS and FEATS from XRS rule.
    # paramtr: $rule (IN): XRS rule, possibly with trailing LF
    # returns: array with 0 (rules doe not have ' -> '), or 3:
    #          [0]: LHS
    #          [1]: RHS
    #          [2]: FEAT (may be empty string)
    #
    ( ( shift() =~ /$RE_LRF_SAFE/o ) ? ( $1, $2, $3 ) : () ); 
}

sub extract_lrf_fast($) {
    # purpose: extract LHS, RHS and FEATS from XRS rule with metadata.
    # paramtr: $rule (IN): XRS rule that must have features.
    # warning: Works only on rules WITH meta-data (features)
    # returns: array with 0 (rules doe not have ' -> '), or 3:
    #          [0]: LHS
    #          [1]: RHS
    #          [2]: FEAT (may be empty string)
    #
    ( ( shift() =~ /$RE_LRF_FAST/o ) ? ( $1, $2, $3 ) : () );
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

sub extract_root_nt($) {
    # purpose: obtain the root NT from a given rule || LHS
    # paramtr: $rule or $lhs (IN): something to extract the NT of
    # returns: root NT
    # warning: This expression is not optimized (yet).
    #
    ( shift() =~ /^\s*([^:() ]+)\(/ ? $1 : '' ); 
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

    my @xrs;
    if ( (@xrs = extract_lrf_fast($_)) ) {
       ..
    }

=head1 DESCRIPTION

This module provides a basic set of tools to parse XRS rules in Perl.
The parsing deals pretty well with strings that have three quotes. 

While Perl regular expressions are powerful and fast, certain scenarios
are still handled faster, if you use plain string functions like
C<index> and C<substr>. Some of the C<*_fast> methods below try to make
use of that. 

However, once you extract multiple items from a rule, which can be
handled by a single regular expression, but only multiple C<index> and
C<substr> method, regular expressions are catching up speed-wise.

=head1 CONSTANTS

=over 4

=item FOREIGN

This string constant exports the foreign sentence marker. When
distinguishing between lexical and non-lexical rules, you should
consider rules with the foreign sentence marker as non-lex. This
constant helps you compare against the marker in your code. 

It is typically a lot faster to use the C<index> function to check
either the full rule, or the RHS, against the C<FOREIGN> marker: 

    if ( index($_,FOREIGN) == -1 ) { ... not a TOP/GLUE rule ... }

=item QUOTED_FOREIGN

Same as C<FOREIGN>, but includes the bracketing double-quotes into
the string. 

=item LHS_RHS_SEPARATOR

This is the separator between the LHS of a rule and the RHS of a
rule. It is B<exactly> space, hypen, greater-than, space. It is B<not>
permissible to substitute either space with another whitespace
character.

Any valid rule B<must> contain this separator. 

=item RHS_FEAT_SEPARATOR

This is the separator between the RHS and the feature list. It is
B<exactly> space, triple-hash, space. It is B<not> permissible to
substitute either space with another whitespace character.

Since features may be optional, a rule may still be valid without this
token. All C<*_fast> methods below assume that this token exists. The
C<*_safe> methods do not assume so.

=item LHS_RHS_LENGTH

This constant is the length of the C<LHS_RHS_SEPARATOR> token. 

=item RHS_FEAT_LENGTH

This constant is the length of the C<RHS_FEAT_SEPARATOR> token. 

=back 

=head1 VARIABLES

This section describes a set of pre-compiled regular expressions which
are exported in the form of variables. You should treat these variables
as I<read-only> or I<constant>. Ideally, if you can live the
(millisecond) call overhead, you may want to use the functions from the
next section instead.

For efficiency, you should avoid applying multiple regular expressions
to the same rule. Use one of the multi-selectors, if you need multiple 
things from a rule. 

When using the regular expressions below, do not forget to add the B<o>
suffix to your regular expression like this: 

    if ( /$RE_RHS_FAST/o ) { ... }

=over 4

=item $RE_LHS_SAFE

This regular expressions returns the LHS of a given rule in C<$1>. 

B<Note:> This expression is identical to C<$RE_LHS_FAST>.

=item $RE_LHS_FAST

This regular expressions returns the LHS of a given rule in C<$1>. 

B<Note:> This expression is identical to C<$RE_LHS_SAFE>.

=item $RE_RHS_FAST

This regular expression returns the RHS of a given rule in C<$1>. It
I<requires> the presence of both, the C<LHS_RHS_SEPARATOR> and
C<RHS_FEAT_SEPARATOR> token in the rule.

=item $RE_RHS_SAFE

This regular expression returns the RHS of a given rule in C<$1>. It
I<requires> the presence of the C<LHS_RHS_SEPARATOR> token. The
C<RHS_FEAT_SEPARATOR> token is optional, and may have an end-of-string
in its place (optional presence of features). 

=item $RE_FEAT_SAFE

This regular expression returns the features of a given rule in C<$1>. 
Since features are optional, the result may be empty, if the rule lacks
the C<RHS_FEAT_SEPARATOR> token.

B<Note:> There is no C<$RE_FEAT_FAST> due to the optionality of features. 

=item $RE_LR_FAST

This regular expression returns the LHS in C<$1> and the RHS in
C<$2>. It I<requires> the presence of both, the C<LHS_RHS_SEPARATOR> and
C<RHS_FEAT_SEPARATOR> token in the rule.

=item $RE_LR_SAFE

This regular expression returns the LHS in C<$1> and the RHS in
C<$2>. It I<requires> the presence of the C<LHS_RHS_SEPARATOR> token. The
C<RHS_FEAT_SEPARATOR> token is optional, and may have an end-of-string
in its place (optional presence of features). 

=item $RE_LRF_FAST

This regular expression returns the LHS in C<$1>, the RHS in C<$2>, and
the feature list string in C<$3>. It I<requires> the presence of both,
the C<LHS_RHS_SEPARATOR> and C<RHS_FEAT_SEPARATOR> token in the rule.

=item $RE_LRF_SAFE

This regular expression returns the LHS in C<$1>, the RHS in C<$2>, and
the feature list string in C<$3>. It I<requires> the presence of the
C<LHS_RHS_SEPARATOR> token. The C<RHS_FEAT_SEPARATOR> token is optional,
and may have an end-of-string in its place (optional presence of
features).

=back

=head1 FUNCTIONS

This section documents the exported functions and helpers. If in doubt,
you should use these functions in your code. Chances are that the API
will be maintained over several revisions, and you can benefit from
bug-fixes and speed improvements without having to change your code.

=over 4

=item [%wc = ] count_words(@words); 

This method counts the occurances of each word in the list of words, and
returns an array indexed by the unique word and their count as value.

=item $root = target_tree($lhs); 

=item $root = target_tree($lhs,'POS'); 

This method returns a hash reference to the root of the tree
representing the LHS of a rule, or C<undef>. The tree is a simple Perl
nested hash structure where each hash represents a node. 

Given a LHS of 

    VP(VBP("have") VP-C(VBN("been") x0:VP-C))

In case of the single argument function, the result looks like

    $root = {'NT' => 'VP','KIDS' => [{'NT' => 'VBP','KIDS' => [{'T' => '"have"'}]},
	{'NT' => 'VP-C','KIDS' => [{'NT' => 'VBN','KIDS' => [{'T' => '"been"'}]},
	{'VAR' => 'x0:VP-C'}]}]};

In case of a two-argument call with second argument C<POS>, the KIDS
hash key remains, but all other keys are uniformely named. Such naming
may ease post-processing:

    $root = {'KIDS' => [{'KIDS' => [{'POS' => '"have"'}],'POS' => 'VBP'},
	{'KIDS' => [{'KIDS' => [{'POS' => '"been"'}],'POS' => 'VBN'},
	{'POS' => 'x0:VP-C'}],'POS' => 'VP-C'}],'POS' => 'VP'};

A back-pointer C<PARENT> is intentionally left off, as you should
process trees using recursive functions, or maintain your own LIFO or
FIFO structures. 

Also note that Perl hashes are typically dimensioned for eight keys
before they are redimensioned. Thus, the tree representation is wasteful
with space. This is not a problem when dealing with one tree at a
time. However, attempting to keep a full rule file's left-hand sides as
trees around requires more consideration.

I have not fully commited to this tree, as the visualization use OO-Perl
techniques to construct a tree. It is feasible to connect those two code
bases in some way.

Note: This function is likely the slowest in the whole module. Use with
caution.

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

If you are interested to B<only> extract target language words from
rules in a speedy manner, you should invoke C<target_words> directly
from the C<extract_lhs_fast> function:

    my ($lhs,@target);
    while ( ... ) {
        if ( ($lhs=extract_lhs_fast($_)) ) {
            my @target = target_words($lhs);

If you require I<unique> words, try 

            my @uniq = keys %{{ map { $_ => 1 } target_words($1) }};

If you need to count the unique words, to handle multiple occurances of
the same word, try:

            my %uniq = count_words( target_words($lhs) );

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

If you are interested to B<only> extract source language words from
rules in a speedy manner, you should invoke C<source_words> directly on
the C<extract_rhs_safe> return value. If B<all> your rules B<always>
have features, then it is safe to use the C<extract_rhs_fast> function,
which is faster than C<extract_rhs_safe>.

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
         $vars[$index] += 1; # will burp if -w or use warnings
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
non-lexical rules: Non-lexical rules generate the lone output
C<x0>. Please note that the C<FOREIGN> sentence marker, by this means, is
considered lexical. 

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

If you are interested to B<only> extract the feature map from rules in a
speedy manner, you should invoke C<feature_spec> directly from the
following looped expression:

    while ( ... ) {
        if ( ($f=extract_feat_safe($_)) ) {
            my %feats = feature_spec($f);

or, even faster, if you don't mind new-lines: 

    while ( ... ) {
        if ( ($p = index($_,RHS_FEAT_SEPARATOR)) > 0 ) {
            my %feats = feature_spec(substr($_,$p+RHS_FEAT_LENGTH));

The latter expression is implemented by the C<extract_feat_fast>
function. You need to make special arrangements, if you can have rules
that don't have any feature specification on them, i.e. above expression
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

If you are interested to B<only> extract the feature map from rules in a
speedy manner, please see C<feature_spec> for details.

=item @xrs = extract_lrf( $rule );

This method extracts the three components, LHS, RHS and FEATS, as
strings from the rule. Trailing line separators in the rule are not a
problem, and do not need to be removed. The components are returned in
that order: LHS, RHS, FEATS.

In case of a bad rule, the returned array is empty. Please note that the
third item may be an empty string, if the rule does not have features.

Alternatively, the pre-compiled regular expression is exported in
variables C<$RE_LRF_SAFE> for faster matching inside your loop.

=item @xrs = extract_lrf_fast( $rule );

This method extracts the three components, LHS, RHS and FEATS, as
strings from the rule. The function has an C<_fast> suffix, because
input rules B<must> contain features. Trailing line separators in the
rule are not a problem, and do not need to be removed. The components
are returned in that order: LHS, RHS, FEATS.

In case of a bad rule, the returned array is empty. This function is
significantly faster than C<extract_lrf>, but may have a more limited
applicability, depending on your use-case. 

Alternatively, the pre-compiled regular expression is exported in
variables C<$RE_LRF_FAST> for faster matching inside your loop.

=back

The next section describes a family of function to solely extract one
part of a rule, e.g. only the RHS or only the LHS. It is imperative to
understand that you should never call two of the C<extract_*_safe> or
C<extract_*_fast> set of functions for the same rule. If you need to
extract more than one of LHS, RHS and FEAT, you should call
C<extract_lrf> or C<extract_lrf_fast> instead. There are also optimized
versions C<extract_lr_safe> and C<extract_lr_fast> for RHS and LHS
simultaneous extractions.

=over 

=item $lhs = extract_lhs_fast( $rule );

This method extracts only the LHS from a given rule. The return value of
the function is the LHS string, which may be empty. Empty strings
evaluate to C<false> in Perl.

While the pre-compiled regular express C<$RE_LHS_FAST> is exported, it
is usually faster to work with C<index> and C<substr>, like this
function, to extract just the LHS from a rule.

The LHS is independent of the I<safe> or I<fast> distinction, so you can
use C<$RE_LHS_SAFE> to extract the LHS.

=item $lhs = extract_lhs_safe( $rule );

This method extracts only the LHS from a given rule. The return value of
the function is the LHS string, which may be empty. Empty strings
evaluate to C<false> in Perl.

Alternatively, the pre-compiled regular expression is exported in
variables C<$RE_LHS_SAFE> for matching inside your loop.

=item $rhs = extract_rhs_fast( $rule );

This method extracts only the RHS from a given B<full> rule B<with>
meta-data. The return value of the function is the RHS string, which may
be empty. Empty strings evaluate to C<false> in Perl.

B<Warning>: The input rule to this function must have features. If
features are missing, the result will have any trailing junk, including
LF. However, for virtually all of ISI rule processing, it is safe to
assume that rules will have features.

Alternatively, the pre-compiled regular expression is exported in
variables C<$RE_RHS_FAST> for matching inside your loop.

=item $rhs = extract_rhs_safe( $rule );

This method extracts only the RHS from a given rule with or without
features. The return value of the function is the RHS string, which may
be empty. Empty strings evaluate to C<false> in Perl.

This function is I<a lot> slower than the C<extract_rhs_fast>
function. However, unlike C<extract_rhs_fast>, it is safe to use with
rule that may or may not have features attached to them.

Alternatively, the pre-compiled regular expression is exported in
variables C<$RE_RHS_SAFE> for matching inside your loop.

=item $feat = extract_feat_safe( $rule );

This method extracts only the FEAT from a given rule with or without
features. The return value of the function is the FEAT string, which may
be empty. Empty strings evaluate to C<false> in Perl.

Alternatively, the pre-compiled regular expression is exported in
variables C<$RE_FEAT_SAFE> for matching inside your loop.

=item $feat = extract_feat_fast( $rule );

This method extracts only the FEAT from a given rule with or without
features. The return value of the function is the FEAT string, which may
be empty. Empty strings evaluate to C<false> in Perl. 

B<Note:> This method B<requires> that the input is already
C<chomp>ed. Unfortunately, it is only marginally faster than
C<extract_feat_safe>, if you have to specifically C<chomp>
rules. However, if your input is already without trailing line
terminator, you will be slightly faster. 

=item $lhs = extract_lr_fast( $rule );

This method extracts the LHS and RHS from a given rule. The return value
of the function an array with the LHS string and the RHS string, either
of which may be empty. Umatched rules return an empty array, which
evaluates to C<false> in Perl.

B<Warning>: The input rule to this function must have features, or it
will fail. However, for virtually all of ISI rule processing, it is safe
to assume that rules will have features.

Alternatively, the pre-compiled regular expression is exported in
variables C<$RE_LR_FAST> for marginally faster matching inside your loop.

=item $lhs = extract_lr_safe( $rule );

This method extracts the LHS and RHS from a given rule. The return value
of the function is an array with the LHS string and the RHS string,
either of which may be empty. Umatched rules return an empty array, which
evaluates to C<false> in Perl.

Alternatively, the pre-compiled regular expression is exported in
variables C<$RE_LR_SAFE> for marginally faster matching inside your loop.

=back

The C<$RE_LHS_FAST> and C<$RE_LHS_SAFE> pre-compiled regular expression
are identical, because they don't depend on the presence or absence of
features. There is no C<$RE_FEAT_FAST> pre-compiled regular expression,
because the match is done without regular expressions.

If you intend to use the pre-compiled regular expressions, you should
use the C<o> suffix in order to keep Perl from re-compiling the
expression in each iteration:

    while ( <R> ) {
       if ( /$RE_LHS_FAST/o ) {
          my @words = target_words($1); 
       }
    }

=over

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

=head1 THANKS

David Chiang, Steve DeNeefe, Michael Pust, Wei Wang. 

=head1 RECOMMENDATIONS

I suggest to use the functions as exported by this module, and not the
regular expression variables. I expect that the function API does not
change as new data becomes available. In other words, once I release a
new version of this module, if you use the functions, you may not have
to update your code to benefit from potential speed upgrades.

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
