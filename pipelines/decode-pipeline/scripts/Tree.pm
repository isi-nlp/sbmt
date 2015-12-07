#
# derivation tree parser, based on Jon May's code.
#
package Tree;
use strict;
use vars qw($debug $AUTOLOAD);

require Exporter;
our @ISA = qw(Exporter);

our $maxcount = 10000;		# cut-off for rule count display
sub prepare_rules_for_augment(\%@);	# { make emacs happy }

our @EXPORT_OK = qw(prepare_rules_for_augment $maxcount);
our %EXPORT_TAGS = ( 'all' => [ @EXPORT_OK ] );
our @EXPORT = qw();

our $VERSION = 1.00;
$VERSION = $1 if ( '$Revision$' =~ /Revision:\s+([0-9.]+)/ );

use Carp;
use POSIX qw(ceil floor);

sub new { 
    # purpose: given a string, return a hash form of the tree
    # paramtr: $string (IN): string representation of tree
    # returns: instance of tree
    #
    my $proto = shift;
    my $class = $proto || ref $proto || __PACKAGE__;
    bless { KIDS => undef, @_ }, $class;
}

sub clone {
    my $self = shift;
    my $result = { %{$self} };
    delete $result->{KIDS};
    foreach my $kid ( $self->kids ) {
	push( @{$result->{KIDS}}, $kid->clone );
    }
    bless( $result, ref $self );
}

my $posrx = qr{[[:alnum:][:punct:]]};

sub from_tokens(\@$);		# { make emacs happy }
sub from_tokens(\@$) {
    # purpose: parse Lisp-like tree syntax
    # paramtr: @token (IO): array of tokens to process
    # returns: (sub-) tree as instance of $class
    #
    my $tokens = shift;		# array passed by reference
    my $key = shift;
    my $token = shift @$tokens;
    confess "my presumption is wrong" unless $token =~ $posrx;

    # create new sub-tree root
    my $root = Tree->new( $key => $token );
    for ( $token=shift(@$tokens); $token ne ')'; $token=shift(@$tokens) ) {
        if ( $token eq '(' ) {
	    # child is a tree; recurse and add to children of current root
            push( @{$root->{KIDS}}, from_tokens(@$tokens,$key) );
        } elsif ( $token =~ $posrx ) {
	    # child is a leaf; add it
            push( @{$root->{KIDS}}, Tree->new( $key => $token ) );
        } else {
	    croak "Odd token found: $token";
	}
    }

    # return sub-tree (or full tree)
    $root;
}

sub lisplike($) { 
    # purpose: given a string, return a hash form of the tree
    # warning: This is a constructor! Call as Tree->lisplike( blah )
    # paramtr: $string (IN): Lisp-like string representation of tree
    # returns: instance of tree
    #
    
    # if called in OO syntax, first argument is class
    shift if UNIVERSAL::isa( $_[0], __PACKAGE__ );

    # first real argument
    local $_ = shift;
    s/^\s+//;			# trim front
    s/\s+$//;			# trim rear
    # FIXME: This will fail, if input has terminals like "(..)"
    s{([()])}{ $1 }g;		# parenthesis tokens
    
    my @tokens = split ;
    shift @tokens eq '(' || confess "Illegal string";
    from_tokens(@tokens,'LABEL');
}

sub functional($) { 
    # purpose: given a string, return a hash form of the tree
    # warning: This is a constructor! Call as Tree->functional( blah )
    # paramtr: $string (IN): Operator string representation of tree
    # returns: instance of tree
    #
        
    # if called in OO syntax, first argument is class
    shift if UNIVERSAL::isa( $_[0], __PACKAGE__ );

    # 
    # inspired by Michael Pust
    #
    my $lhs = shift; 
    my ($root,$x,@stack); 
    foreach ( split ' ', $lhs ) {
	# FIXME: The string :(";") is a valid LHS !!!
	#while ( /(x\d+:[^ ()""]+)|(\".*?\")(?=\)+(?:\s|$))|([^ :()""]+)\(|(\))/g ) {
	while ( /(x\d+:[^ ()""]+)|(\".*?\")(?=\)+(?:\s|$))|([^ ()""]+)\(|(\))/g ) {
	    if ( ($x = $1 || $2) ) {
		# variable or terminal leaf
		push( @{$root->{KIDS}}, Tree->new( POS => $x ) ); 
	    } elsif ( $3 ) {
		# non-terminal
		my $tree = Tree->new( POS => $3 ); 
		push( @{$root->{KIDS}}, $tree ) if defined $root; 
		push( @stack, $root ); 
		$root = $tree; 
	    } elsif ( $4 ) {
		# level up
		die "unbalanced parentheses in expression >>$4<< of >>$_<< of >>$lhs<<" unless @stack; 
		$root = pop(@stack) if defined $stack[$#stack]; 
	    } else {
		confess "Unable to match token in rule";
	    }
	}
    }

    $root; 
}

sub label {
    # purpuse: getter/setter for LABEL component
    # paramtr: $new_label (opt. IN): in setter mode, the new label
    # returns: old (setter) or current (getter) label
    #
    my $self = shift;
    my $result = $self->{LABEL};
    $self->{LABEL} = shift if @_;
    $result;
}

sub pos {
    # purpuse: getter/setter for POS (part of speech) component
    # paramtr: $new_pos (opt. IN): in setter mode, the new PoS
    # returns: old (setter) or current (getter) PoS
    #
    my $self = shift;
    my $result = $self->{POS};
    $self->{POS} = shift if @_;
    $result;
}

sub kids {
    # purpose: getter for KIDS component
    # paramtr: replacement children as array or array-reference
    # returns: possibly empty list of children
    #          undef or child, if called with argument
    #
    my $self = shift;
    my @result = exists $self->{KIDS} && defined $self->{KIDS} ? 
	@{ $self->{KIDS} } : 
	();

    if ( @_ ) {
	# setter
	if ( ref $_[0] eq 'ARRAY' ) {
	    # passing array reference -- assume only argument
	    $self->{KIDS} = shift;
	} else {
	    # passing array -- any number of arguments
	    $self->{KIDS} = [ @_ ];
	}
    }

    @result; 
}

sub child {
    # purpose: getter for KIDS component
    # paramtr: $index (IN): which single child to obtain
    # returns: undef or child, if called with argument
    #
    my $self = shift;
    my $index = shift() + 0;

    # argument is the index
    return undef unless exists $self->{KIDS} && defined $self->{KIDS};
    $self->{KIDS}->[ $index ];
}

sub to_dtree {
    # purpose: convert only derivation tree back into string notation
    # paramtr: $neworder (opt. IN): If defined and true, print nonmonotone d-tree order
    #                               If unset, print original d-tree order. 
    # returns: string representing d-tree in Lisp-like format
    # warning: an augmented tree may have nodes w/o label. These need to be skipped. 
    # warning: an augmented tree will have a re-ordered d-tree (nonmonotone)!
    #
    my $self = shift;

    # for nonmonotone rule-augmented trees, recall original dtree ordering!
    # but only, if we didn't request the re-ordered tree!
    my $neworder = shift;
    $self = ${$self->{ORIG}} if exists $self->{ORIG} && defined $neworder && $neworder;

    if ( $self->kids == 0 ) {
	# this is it
	$self->label || '';
    } else {		
	my @kids = $self->kids; 

	my $result = '';
	if ( defined $self->label ) { 
	    $result = '(' . $self->label;
	    my $children = join( ' ', grep { $_ } map { $_->to_dtree($neworder) } $self->kids );
	    $result .= " $children" if $children;
	    $result .= ')';
	    $result = substr( $result, 1, -1 ) unless $children;
	} else {
	    $result = join( ' ', grep { $_ } map { $_->to_dtree($neworder) } $self->kids );
	}
	$result; 
    }
}

sub to_ptree {
    # purpose: convert only PoS tree back into string notation
    # returns: string representing PoS-tree in functional format
    # warning: there is no PoS output for a non-augmented d-tree
    #
    my $self = shift;
    if ( $self->kids == 0 ) {
	# this is it
	my $result = $self->pos;
	confess "The tree you are trying to print is not augmented"
	    unless defined $result;
	$result;
    } else {
	# I don't expect an augmented tree to have levels w/o pos
	$self->pos . '(' . 
	    join( ' ', map { $_->to_ptree } $self->kids ) . ')';
    }
}

sub _find_leaves_with_x {
    # purpose: internal helper function to find all leaves with xNN:PoS
    # paramtr: $aref (IO): array reference to be filled with leaf references
    # 
    my $self = shift;
    my $aref = shift;

    if ( $self->pos =~ /^x(\d+)/ ) {
	$aref->[$1] = $self;
    } else {
	foreach my $kid ( $self->kids ) {
	    $kid->_find_leaves_with_x($aref);
	}
    }
}

sub find_leaves_with_x {
    # purpose: finds all leaves where the PoS references a variable
    # returns: array with references to leaves, ordered by variable 
    # warning: Only apply this method to LHS trees, never your d-tree. 
    #
    my $self = shift;
    my @result = ();
    $self->_find_leaves_with_x( \@result );
    @result;
}

sub _nodeid {
    my $self = shift;
    my $map = shift;
    if ( exists $map->{$self} ) {
	$map->{$self};
    } else {
	$map->{$self} = ++$map->{__count}; 
    }
}

sub _escape($) {
    local $_ = shift;
    s{([\"])}{\\$1}g;
    "$_";
}

sub _serialize(\%;$) {
    my $href = shift;
    my $separator = shift || ',';
    my @result = ();
    while ( my ($k,$v) = each %{$href} ) {
	push( @result, "$k=$v" );
    }
    join( $separator, @result );
}

sub has_attr {
    my $self = shift;
    my $key = shift;

    exists $self->{ATTR} ? exists $self->{ATTR}->{$key} : 0;
}

sub inner_graphviz {
    my $self = shift;
    my $map = shift;
    my $indent = shift;

    my $nodeid = $self->_nodeid($map);
    my $result = "$indent  $nodeid ";
    my @kids = $self->kids;

    # user attributes control the rendering of nodes.
    my $user = $map->{'__user'}; 
    my %attr = exists $user->{__all} ? %{$user->{__all}} : (); 
    if ( defined $self->label ) {
	# _root node (of a rule)
	%attr = ( %attr, %{$user->{_root}} ) if exists $user->{_root};
    } elsif ( @kids == 0 ) {
	# _leaf node
	%attr = ( %attr, %{$user->{_leaf}} ) if exists $user->{_leaf};
    } else {
	# _inner node
	%attr = ( %attr, %{$user->{_inner}} ) if exists $user->{_inner};
    }

    if ( defined $self->label ) {
	# _root node (of a rule)
	my $pos = $self->pos;
	my $label = $self->label;

	foreach my $key ( keys %{$user} ) {
	    next if substr($key,0,1) eq '_'; # already dealt with
	    %attr = ( %attr, %{$user->{$key}} ) if $self->has_attr($key);
	}

	# serialize 
	my $extra = _serialize(%attr);
	substr( $extra, 0, 0, ',' ) if $extra; # prepend comma, if content
	$result .= "[label=<
<TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"0\">
<TR><TD>$pos</TD></TR>
<TR><TD BGCOLOR=\"\#FFFFAA\"><FONT POINT-SIZE=\"10\">$label</FONT></TD></TR>
</TABLE>>$extra]\n";
    } else {
	# _leaf or _inner
	my $extra = _serialize(%attr);
	substr( $extra, 0, 0, ',' ) if $extra; # prepend comma, if content
	$result .= "[label=\"" . _escape($self->pos) . "\"$extra];\n";
    }

    foreach my $kid ( @kids ) {
	$result .= "$indent  " . $self->_nodeid($map) . ' -> '; 
	$result .= $kid->_nodeid($map) . ";\n";
	$result .= $kid->inner_graphviz($map,$indent . '  ');
    }

    $result;
}

sub graphviz {
    # purpose: convert p-tree into graphviz format
    # paramtr: %user (IN): map to describe how _root, _leaf, _inner rules are
    #          default, and how any feature changes the rendering of a _root. 
    #          See http://www.graphviz.org/doc/schema/attributes.xml
    # returns: string with graphviz rendering of p-tree
    #
    my $self = shift;
    my $map = { __count => 0, 
		__user => { 
		    _root => { color => '"#0000AA"', shape => 'box' }
		  , _leaf => { fontname => 'Helvetica' }
		  , is_lexicalized => { color => '"#4444FF"', shape => 'Mrecord' }
		  , green => { color => '"#008B45"' }
		  , maroon => { color => '"#B03060"' }
		  , olive => { style => 'fill', fillcolor => '"#C0FF3E"' }
		  , nonmonotone => { style => 'filled', fillcolor => '"#FFCCCC"' }
		  # next must be last, to permit user to overwrite any above
		  , @_ 
		} }; 

    "strict digraph { ordering=out\n" . 
	"  edge [arrowhead=none];\n" . 
	"  node [shape=none,fontsize=12];\n" . 
	$self->inner_graphviz($map,'  ') . 
	"}\n";
}


sub _munge(\$$$) {
    # purpose: replace whitespace in textual-value attributes with underscore
    # paramtr: $attr  (IO): attributes string of rule
    #          $outer (IN): value portion enclosed by braces
    #          $inner (IN): value portion without braces
    #
    my ($sref,$outer,$inner) = @_;
    $inner =~ s/\s+/_/g;
    substr( $$sref, index( $$sref, $outer ), length($outer), $inner );
}

my @integer_features = qw(count derivation-size foreign-length is_lexicalized
			  missingWord spuriousWord size countone hashid
			  text-length foreign-length nomodel1inv nomodel1nrm
			  unk-rule glue-rule 
			  lm1-unk lm2-unk lm-open-unk lm-unk);

sub _sane_fval(\%) {
    # purpose: extract feature values, minding integer attributes
    #          that grammar_view may have obscured
    #
    my $href = shift; 
    foreach my $k ( @integer_features ) {
	next unless exists $href->{$k}; 
	$href->{$k} = -substr($href->{$k},3) if substr($href->{$k},0,3) eq '10^';
    }
}

sub prepare_rules_for_augment(\%@) {
    # purpose: for augment method, prepare XRs rules into parsed format
    # warning: This is the static (class) method
    # paramtr: %rules (IO): hash to put rules into, tied hashes permitted
    #          @rules (IN): XRs rules, <filehandle> permitted
    # actions: hash { ruleid => [ lh-tree, rh-xindex-aref, attr-href ]
    # returns: number of rules inserted

    # ignore 1st arg, if we were called as instance or class method
    shift if ( UNIVERSAL::isa( $_[0], __PACKAGE__ ) || $_[0] eq __PACKAGE__ );

    my $rules = shift;
    confess "prepare_rules_for_augment is invoked incorrectly" 
	unless ref $rules eq 'HASH'; # type check

    my $n = 0;
    foreach ( @_ ) {
	s/[\015\012]+$//;	# optionally remove line terminator
	if ( /(.*?) -> (.*?) \#\#\# (.*)/ ) {
	    my ($lhs,$rhs,$attr) = ($1,$2,$3);

	    my $ltree = Tree->functional($lhs) || die;
	    my @rhs = map { substr($_,1)+0 } grep { /^x/ } split /\s+/, $rhs; 
	    _munge($attr,$1,$2) while ( $attr =~ /=(\{\{\{(.*?)\}\}\})/g );
	    _munge($attr,$1,$2) while ( $attr =~ /=(\{(.*?)\})/g );
	    my %attr = map { split /=/, $_, 2 } grep { /=/ } split /\s+/, $attr;
	    _sane_fval(%attr); 	# grammar_view produces 10^-int values :-(

	    $rules->{ $attr{id} } = [ $ltree, \@rhs, \%attr ];
	    $n++;
	} else {
	    warn "Warning: Ignoring rule $_\n";
	}
    }

    $n;
}

sub augment {
    my $self = shift;
    my $rule = shift;
    confess( "ERROR: Rule ", $self->label, " not found" )
	unless exists $rule->{ $self->label };

    # augment children first -- depth first
    foreach my $kid ( $self->kids ) {
	$kid->augment($rule);
    }

    my ($lhs,$rhs,$attr) = @{ $rule->{ $self->label } };
    $lhs = $lhs->clone;		# IMPORTANT: The same rule may apply multiple times!


    if ( $self->kids == 0  ) {
	# no children, unconditionally augment, no worries
	$self->pos( $lhs->pos );
	$self->kids( $lhs->kids );
	$self->{ATTR} = { %{$attr} }; # deep copy
    } else {
	# complex augmentation
	my @leaves = $lhs->find_leaves_with_x;
	# return unless @leaves;

	$self->pos( $lhs->pos );
	my @kids = $self->kids;
	$self->kids( $lhs->kids );

	# we need as many p-tree leaves with 'x' as we have d-tree children
	# we need as many p-tree leaves with 'x' as there are 'x' on the RHS. 
	# this assertion likely only works when augmenting the tree bottom-up. 
	if ( @leaves != @kids || @leaves != @{$rhs} ) {
	    confess( @leaves+0, " leaves, ", 
		     @kids+0, " kids, ",
		     @{$rhs}+0, " rhs\n",
		     Data::Dumper->Dump(  [$self,$lhs,$rhs,$attr],
				       [qw($self $lhs $rhs $attr)] ), "\n" );
	}

	# FIXME: Can we trust rules to be marked nonmonotone? 
	# For now, trust presence of nonmonotone attribute... 
	for ( my $i=0; $i < @kids; ++$i ) {
	    my $j = $rhs->[$i]; # re-order
	    $leaves[$j]->label( $kids[$i]->label );
	    $leaves[$j]->kids( $kids[$i]->kids );
	    $leaves[$j]->{POS} =~ s{^x(\d+):}{};
	    # FIXME: The hash lookup may fail, if there is no such key
	    $leaves[$j]->{ATTR} = { %{ $rule->{ $kids[$i]->label }->[2] } };

	    # ONLY remember original d-tree order when necessary (nonmonotone)
	    $leaves[$i]->{ORIG} = \$kids[$i] if exists $attr->{nonmonotone}; 
	}
    }
}

sub AUTOLOAD {
    my $self = shift;
    my $type = ref($self) ||
        confess "\"$self\" is not an object (\$AUTOLOAD=$AUTOLOAD)";

    my $name = uc($AUTOLOAD);
    $name =~ s/.*:://;   # strip fully-qualified portion
    warn( "# ", __PACKAGE__, "::AUTLOAD invoked for $name\n" ) if $debug;

    # avoid auto-vivification
    my $result = exists $self->{$name} ? $self->{$name} : undef;
    $self->{$name} = shift if (@_);
    $result;
}

sub nodecount {
    # purpose: count number of nodes from this node down
    # returns: node count, always at least 1
    #
    my $self = shift;
    my $n = 1;
    foreach my $kid ( $self->kids ) {
	$n += $kid->nodecount;
    }
    $n;
}

sub depth {
    # purpose: determine height of (sub-)tree
    # returns: number of edges
    # warning: does breadth-first search
    #
    my $self = shift;
    my $n = 0;
    for ( my @q = ( $self ); @q; ++$n ) {
	@q = map { $_->kids } @q;
    }
    $n;
}

sub leaves {
    # purpose: deep-find all leaves from given root
    # returns: list of leaves nodes, may be empty
    #
    my $self = shift;
    if ( exists $self->{KIDS} && defined $self->{KIDS} ) {
	# not a leaf -- collect leaves from children
	( map { $_->leaves } $self->kids );
    } else {
	# isa leaf -- return self
	( $self );
    }
}

sub subroots {
    # purpose: find all leaves that are start of other sub-trees
    # returns: array with these leaves, may be empty
    #
    my $self = shift;
    my @result = ();
    foreach my $kid ( $self->kids ) {
	if ( $kid->label ) {
	    push( @result, $kid );
	} else {
	    push( @result, $kid->subroots );
	}
    }
    @result;
}

sub show_assignment {
    # purpose: show the assigned coordinates for each level
    # paramtr: $fh (IN): open file handle to print to
    # 
    my $self = shift;
    my $fh = shift;

    # level-order traversal
    my @q = ( $self );
    while ( @q > 0 ) {
	my $s = join( ',', map { sprintf("%.1f",$_->{X}) } @q );
	print $fh "y=", $q[0]->{Y}, ", x=[$s]\n";
	@q = map { $_->kids } @q; 
    }
    print $fh "\n";
}

sub assign_hue {
    my $self = shift;
    my $hue = $self->hue; 	# root must be assigned manually
    my @subs = $self->subroots ;

    # first assign hues
    for ( my $n=0; $n < @subs; ++$n ) {
	$hue += 0.15;
	$hue = 0 if $hue >= 1.0;
	if ( ($n & 1) == 0 ) {
	    my $x = $hue + 0.5;
	    $x -= 1.0 if $x >= 1.0; 
	    $subs[$n]->hue($x);
	} else {
	    $subs[$n]->hue($hue);
	}
    }

    # then recurse
    foreach my $kid ( @subs ) {
	$kid->assign_hue; 
    }
}

#
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# ! DO NOT USE CODE BELOW THIS LINE. IT'S IN PROGRESS AND MAY NOT WORK !
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#

sub max(@) {
    my $n = shift;
    foreach my $x ( @_ ) {
	$n = $x if $x > $n;
    }
    $n;
}

sub _assign_leaves {
    my $self = shift;
    my $lref = shift;
    my $level = shift;
    $self->{Y} = $level;

    if ( defined $self->{KIDS} ) {
	# not a leaf -- collect leaves
	( map { $_->_assign_leaves($lref,$level+1) } $self->kids );
    } else {
	# isa leaf -- return self
	$self->{X} = $$lref;
	if ( defined $self->label ) {
	    # either pure d-tree, or root of rule
	    $self->{W} = max( length($self->label || ''), length($self->pos || '') );
	    $$lref += $self->{W} + 1; 
	} else {
	    $$lref += ( $self->{W} = length($self->pos) ) + 1;
	}

	( $self );
    }
}

sub _assign_inner {
    my $self = shift;
    return unless defined $self->{KIDS}; # already did leaves

    my $n = 0;
    my $sum = 0;
    foreach my $kid ( $self->kids ) {
	$kid->_assign_inner ; # unless exists $kid->{X}; 
	$sum += $kid->{X};
	$n++;
    }
    $self->{X} = $n ? int( $sum / $n ) : 0;
    if ( defined $self->label ) {
	# either pure d-tree, or root of rule
	$self->{W} = max( length($self->label), length($self->pos || '') );
    } else {
	$self->{W} = length($self->pos);
    }
}

sub undo_assign {
    my $self = shift;
    delete $self->{X};
    delete $self->{Y};
    delete $self->{W};
    foreach my $kid ( $self->kids ) {
	$kid->undo_assign;
    }
}

sub wide_assign_position {
    my $self = shift;

    # undo any previous positional assignments
    $self->undo_assign; 

    # assign first temporary coordinate to leaves only
    # also assign level (y) to all nodes while at it
    my $len = 0;
    my @leaves = $self->_assign_leaves(\$len,0);

    # update parents of assigned leaves with arith. mean
    $self->_assign_inner ;

    # Can I trust that no overlaps happen at each level? Frankly, I
    # don't know, and I don't trust this, so I'll be inefficient.
    # level-order traversal (using a heap in array @q) to check that no
    # X is less than its left level-neighbor.
    my @q = ( $self );
    my $p_dtree = ( defined $self->label && ! defined $self->pos );
    my $y = 0;
    my $c = 1 + $p_dtree;
    while ( @q ) {
	for ( my $i=1; $i < @q; ++$i ) {
	    my $r = $q[$i-1]->right;
	    my $l = $q[$i]->left;
	    if ( $l - $r < $c ) {
		## warn "# level $y: [",$i-1,"]=$l too close to [$i]=$r, adjusting\n";
		$q[$i]->adjust_x( ($r - $l) + $c );
	    }
	}
	
	@q = map { $_->kids } @q;
	++$y; 
    }

    # add some padding (y-scaling is much larger than x-scaling).
    my @bb = $self->bounding_box(1);
    $self->{__BBOX} = [ $bb[0]-0.5, $bb[1]+0.1, $bb[2]+0.5, $bb[3]-0.1 ];
}

sub tight_assign_position {
    my $self = shift;

    # undo any previous positional assignments
    $self->undo_assign; 
    $self->_kevin(0);

    # add some padding (y-scaling is much larger than x-scaling).
    my @bb = $self->bounding_box(1);
    $self->{__BBOX} = [ $bb[0]-0.5, $bb[1]+0.1, $bb[2]+0.5, $bb[3]-0.1 ];
}




sub left {
    my $self = shift;
#    my $f = shift || ( exists $self->{KIDS} ? ( $self->label ? 0.7 : 1.2 ) : 0.5 );
    my $f = shift || ( $self->kids ? ( $self->label ? 0.7 : 1.2 ) : 0.5 );
    $self->{X} - $self->{W} * $f;
}

sub right {
    my $self = shift;
#    my $f = shift || ( exists $self->{KIDS} ? ( $self->label ? 0.7 : 1.2 ) : 0.5 );
    my $f = shift || ( $self->kids ? ( $self->label ? 0.7 : 1.2 ) : 0.5 );
    $self->{X} + $self->{W} * $f;
}

sub upper {
    my $self = shift;
    my $f = shift || 0.5;
    $self->{Y} - $f;
}

sub lower {
    my $self = shift;
    my $f = shift || 0.5;
    $self->{Y} + $f;
}

sub own_bbox {
    # purpose: the node's own bounding box, no children
    # returns: 4-value array of bounding box for node
    #
    my $self = shift;
    ( $self->left, $self->lower, $self->right, $self->upper );
}

sub bounding_box {
    # purpose: determine bounding box of sub-tree until leaves or new roots
    # paramtr: $force (opt. IN): If set, always go down to leaves, don't stop at subroots
    # returns: bounding box in model coordinates as 4-item array
    #
    my $self = shift;
    my $force = shift || 0;	# optional
    my @bbox = $self->own_bbox;

    foreach my $kid ( $self->kids ) {
	my @b = defined $kid->label && ! $force ? $kid->own_bbox : $kid->bounding_box($force);
	$bbox[0]=$b[0] if $b[0] < $bbox[0];
	$bbox[1]=$b[1] if $b[1] > $bbox[1];
	$bbox[2]=$b[2] if $b[2] > $bbox[2];
	$bbox[3]=$b[3] if $b[3] < $bbox[3];
    }
    @bbox;
}

sub level_bounding_box {
    # purpose: determine bounding box of each sub-tree level until leaves or new roots
    # paramtr: $force (opt. IN): If set, always go down to leaves, don't stop at subroots
    # returns: bounding boxes for each level in model coordinates as array of 4-item arrays
    #
    my $self = shift;
    my $force = shift || 0;	# optional
    my @bbox = ( [ $self->own_bbox ] );

    foreach my $kid ( $self->kids ) {
	my @b = defined $kid->label && ! $force ? 
	    ( [ $kid->own_bbox ] ) : $kid->level_bounding_box($force);
	for ( my $i=0; $i < @b; ++$i ) {
	    if ( defined $bbox[$i+1] ) {
		$bbox[$i+1][0]=$b[$i][0] if $b[$i][0] < $bbox[$i+1][0];
		$bbox[$i+1][1]=$b[$i][1] if $b[$i][1] > $bbox[$i+1][1];
		$bbox[$i+1][2]=$b[$i][2] if $b[$i][2] > $bbox[$i+1][2];
		$bbox[$i+1][3]=$b[$i][3] if $b[$i][3] < $bbox[$i+1][3];
	    } else {
		$bbox[$i+1] = $b[$i];
	    }
	}
    }

    @bbox;
}

sub adjust_x {
    my $self = shift;
    my $adjust = shift;
    $self->{X} += $adjust;
    foreach my $kid ( $self->kids ) {
	$kid->adjust_x($adjust);
    }
}

sub _kevin {
    my $self = shift;
    my $level = shift || 0;
    my $width = max( length($self->pos || ''), length($self->label || '') );
    $self->{W} = max( $width, 2 );
    $self->{Y} = $level;
#    my $f = exists $self->{KIDS} ? ( $self->label ? 0.7 : 1.2 ) : 0.7;
    my $f = $self->kids ? ( $self->label ? 0.7 : 1.2 ) : 0.7;

    my @clbb = ();
    foreach my $kid ( $self->kids ) {
	push( @clbb, [ $kid->_kevin($level+1) ] ); 
    }

    my @result; 
    if ( @clbb == 0 ) {
	# no children, yahoo
	@result = ( [ 0, $level, $width, $level+1 ] );
    } elsif ( @clbb == 1 ) {
	# just one child, try centering on top
	my $cx = ($clbb[0][0][0] + $clbb[0][0][2]) / 2;
	@result = ( [ $cx - $width*$f, $level, $cx + $width*$f, $level+1 ], @{$clbb[0]} );
    } else {
	# multiple children, now this is the difficult part
	
	# phase 1: look at two trees at a time, from left to right
	# for each tree, look at *all* its left neighbors successively
	# for each level, determine the distance, and remember smallest distance
	for ( my $i=1; $i < @clbb; ++$i ) {
	    # phase 1.1: determine closest distance between any two neighbor levels
	    my $shift = 1E6; 
	    for ( my $ii=0; $ii < $i; ++$ii ) {
		for ( my $j=0; $j < @{$clbb[$i]}; ++$j ) {
		    last unless defined $clbb[$ii][$j];
		    my $distance = $clbb[$i][$j][0] - $clbb[$ii][$j][2];
		    $shift = $distance if $shift > $distance; # keep min 
		}
	    }

	    # if not perfectly (1) aligned, adjust the right tree recursively 
	    if ( abs($shift-1) >= 0.5 ) {
		$shift -= 1; 	# extra space
		# phase 1.2: move right tree closer to left (or away, if negative)
		for ( my $j=0; $j < @{$clbb[$i]}; ++$j ) {
		    $clbb[$i][$j][0] -= $shift;
		    $clbb[$i][$j][2] -= $shift;
		}
		$self->child($i)->adjust_x( -$shift );
	    }
	}

	# phase 2: combine all same-level bounding boxes
	my @lbb = ();
	my $deepest = max( map { $_->depth } $self->kids );
	for ( my $j=0; $j <= $deepest; ++$j ) {
	    my ($x1,$x2) = (undef,undef);
	    for ( my $i=0; $i < @clbb; ++$i ) {
		if ( defined $clbb[$i][$j] ) {
		    $x1 = $clbb[$i][$j][0] 
			if ( ! defined $x1 || $x1 > $clbb[$i][$j][0] );
		    $x2 = $clbb[$i][$j][2] 
			if ( ! defined $x2 || $x2 < $clbb[$i][$j][2] );
		}
	    }
	    $lbb[$j] = [ $x1, $level+$j, $x2, $level+$j+1 ] 
		if ( defined $x1 && defined $x2 );
	}

	# phase 3: center parent over all children
	my $cx = ($lbb[0][0] + $lbb[0][2]) / 2;
	@result = ( [ $cx - $width*$f, $level, $cx + $width*$f, $level+1 ], @lbb );
    }

    $self->{X} = ( $result[0][0] + $result[0][2] ) / 2;
    $self->{Y} = $result[0][1];
    @result;
}


sub _ps_escape($) {
    local $_ = shift;
    s{[^[:print:]]}{}g;
    s{\\}{\\\\}g;
    s{([()])}{\\$1}g;
    s{\@UNKNOWN\@}{<unk>};
    "$_";
}

sub _ps {
    my $self = shift;
    my $fh = shift; 
    my %attr = ( @_ );

    # draw edges first
    my $predicate = 0;
    foreach my $kid ( $self->kids ) {
	printf $fh "%.2f %.2f ", $self->{X}, $self->{Y};
	printf $fh "%.2f %.2f showedge\n", $kid->{X}, $kid->{Y};
	$predicate++;
	$kid->_ps($fh,%attr);
    }
    
    my $s = _ps_escape( $self->pos || $self->label );
    $s = substr($s,1,-1) if ( substr($s,0,1) eq '"' );
    my $p_unk = ( $s eq '<unk>' );
    print $fh "currentrgbcolor 1.0 0 0 setrgbcolor " if $p_unk;
    if ( defined $self->label && defined $self->pos ) {
	my $box = $self->has_attr('is_lexicalized') ? '/rcbox' : '/box';
	print $fh "$box setrootbox\n";
	print $fh "currentlinewidth 2 setlinewidth currentdash [2] 0 setdash\n"
	    if $self->has_attr('nonmonotone');
	print $fh '(', _ps_escape($self->label), ") ";
    }
    print $fh "($s) ", $self->{X}, ' ', $self->{Y}, ' ';
    if ( defined $self->label ) { 
	if ( $attr{colorbox} ) {
	    confess "no hue, use assign_hue first" unless exists $self->{HUE};
	    print $fh "@{[$self->hue]} saturation brightness sethsbcolor ";
	} else {
	    print $fh "1 setgray "; 
	}
	if ( defined $self->pos ) {
	    print $fh "showroot";
	    print $fh " setdash setlinewidth" if $self->has_attr('nonmonotone');
	} else {
	    print $fh "showplain";
	}
    } elsif ( $predicate ) {
	print $fh "showinner";
    } else {
	print $fh "showleaf";
    }
    print $fh " setrgbcolor" if $p_unk;
    print $fh "\n";
}

sub _ps2 {
    my $self = shift;
    my $fh = shift;
    foreach my $kid ( $self->kids ) { 
	$kid->_ps2($fh);
    }

#    $self->_polygon($fh) if defined $self->label;

#    if ( defined $self->label ) {
#	my @b = $self->bounding_box;
#	print $fh "[ $b[0] $b[1] $b[0] $b[3] $b[2] $b[3] $b[2] $b[1] ] polygon\n";
#    }

    my $epsilon = 0.5;
    if ( defined $self->label ) {
	confess "no hue, use assign_hue first" unless exists $self->{HUE};
	print $fh "@{[$self->hue]} saturation brightness sethsbcolor\n";
	my @lbox = $self->level_bounding_box ;

	print $fh '[', $lbox[0][0], ' ', $lbox[0][3], ' ', $lbox[0][2], ' ', $lbox[0][3];
	for ( my $j=1; $j<@lbox; ++$j ) {
	    my @a = @{$lbox[$j-1]};
	    my @b = @{$lbox[$j]};
	    print $fh ' ', $a[2], ' ', $a[1];
	    print $fh ' ', $b[2], ' ', $b[3] if ( abs($a[2] - $b[2]) > $epsilon ); 
	}
	my @c = @{$lbox[$#lbox]};
	print $fh ' ', $c[2], ' ', $c[1], ' ', $c[0], ' ', $c[1]; 
	for ( my $j=$#lbox; $j > 0; --$j ) {
	    my @a = @{$lbox[$j]};
	    my @b = @{$lbox[$j-1]};
	    print $fh ' ', $a[0], ' ', $a[3];
	    print $fh ' ', $b[0], ' ', $b[1] if ( abs($a[0] - $b[0]) > $epsilon );
	}
	print $fh "] polygon\n";
    }
}

sub _ps3 {
    # purpose: add rule count attribute
    #
    my $self = shift;
    my $fh = shift;
    for my $kid ( $self->kids ) {
	$kid->_ps3($fh);
    }
    if ( defined $self->label && $self->has_attr('count') ) {
	# root of sub-tree, try to show count
	my $count = $self->{ATTR}->{count};
	$count = ">$maxcount" if $count > $maxcount;
	printf $fh "($count) %.1f %.1f showcount\n", $self->{X}, $self->{Y};
    }
}

sub generate_ps {
    my $self = shift;
    my $fh = shift;
    my %attr = ( @_ );

    my $xscale = 5;
    my $yscale = 35;
    my $xoffset = 50;
    my $yoffset = 600;

    my $user = $ENV{USER} || $ENV{LOGNAME} || (getpwuid($>))[0] || 'unknown';
    if ( exists $self->{__BBOX} ) {
	my ($x1,$y1,$x2,$y2) = @{ $self->{__BBOX} };
	$x1 = $x1 * $xscale + $xoffset;
	$y1 = $yoffset - $y1 * $yscale;
	$x2 = $x2 * $xscale + $xoffset;
	$y2 = $yoffset - $y2 * $yscale;

	print $fh "%!PS-Adobe-2.0 EPSF-2.0\n";
	print $fh "%%BoundingBox: $x1 $y1 $x2 $y2\n";
    } else {
	print $fh "%!PS-Adobe-2.0\n";
    }
    print $fh "%%Creator: @{[__PACKAGE__]} version $VERSION
%%For: $user
%%CreationDate: @{[scalar localtime]}
%%Pages: 1
%%EndComments
save
%%BeginProlog
40 dict begin
/x { $xscale mul $xoffset add } bind def
/y { $yscale mul $yoffset exch sub } bind def
/xyfix { y exch x exch } bind def
/fontsize 14 def
/rulesize 10 def
/countsize 8 def
/saturation 0.25 def
/brightness 1 def
/radius 5 def
/fshow { 
    dup stringwidth pop -0.5 mul fontsize -0.3 mul rmoveto 
    dup (<unk>) ne /unk exch def
    unk { currenthsbcolor 0 setgray 4 -1 roll } if
    show
    unk { sethsbcolor } if
} bind def
/rootposfont { /Helvetica-Bold findfont fontsize scalefont setfont } bind def
/rootlabelfont { /Helvetica findfont rulesize scalefont setfont } bind def
/leaffont { /Times-Roman findfont fontsize scalefont setfont } bind def
/innerfont { /Helvetica-Bold findfont fontsize scalefont setfont } bind def
/setrootbox { /rootbox exch cvx def } bind def
/box {
    % width height (around current center)
    2 copy -0.5 mul exch -0.5 mul exch rmoveto
    1 index 0 rlineto
    0 exch rlineto
    -1 mul 0 rlineto
    closepath
} bind def
/rcbox {
    % width height 
    10 dict begin %% don't redefine outer x and y
    2 div /h exch def
    2 div /w exch def
    currentpoint /y exch def /x exch def
    x y h add moveto
    x w add radius sub y h add lineto
    x w add y h add x w add y h add radius sub radius arct
    x w add y h sub radius add lineto
    x w add y h sub x w add radius sub y h sub radius arct
    x w sub radius add y h sub lineto
    x w sub y h sub x w sub y h sub radius add radius arct
    x w sub y h add radius sub lineto
    x w sub y h add x w sub radius add y h add radius arct
    closepath
    end
} bind def
/background {
    2 copy newpath moveto
    2 index stringwidth pop 1.05 mul fontsize box
%    currenthsbcolor 1 setgray fill sethsbcolor
} bind def
/backroot {
    2 copy 
    rulesize 0.4 mul sub 
    newpath moveto
    rootposfont 2 index stringwidth pop 
    rootlabelfont 4 index stringwidth pop 
    2 copy gt { pop } { exch pop } ifelse 1.05 mul 
    fontsize rulesize add rootbox
    currenthsbcolor gsave fill grestore 0 setgray stroke sethsbcolor
} bind def
/showroot {
    xyfix
    rootposfont
    backroot
    2 copy 
    moveto 3 -1 roll rootposfont fshow
    rootlabelfont 
    rulesize sub moveto fshow
} bind def
/showleaf {
    xyfix
    leaffont
    background
    0.2 fontsize mul sub
    moveto fshow
} bind def
/showinner {
    xyfix
    innerfont
    background
    0.2 fontsize mul sub
    moveto fshow
} bind def
/showplain {
    xyfix
    rootlabelfont
    background
    0.2 fontsize mul sub
    moveto fshow
} bind def
/showedge {
    newpath xyfix fontsize 2 div add moveto xyfix rulesize sub lineto
    currenthsbcolor 0 setgray 0.3 setlinewidth stroke sethsbcolor
} bind def
/showcount {
    newpath xyfix fontsize add moveto fshow
} bind def
/polygon {
    gsave
    /xy exch def
    newpath
    xy 0 get x xy 1 get y moveto
    2 2 xy length 2 sub
    {
	/i exch def
	xy i get x xy i 1 add get y lineto
    } for
    closepath [2] 0 setdash 1 setlinewidth 
    fill
    grestore
} def
%%EndProlog
/box setrootbox
";

    $self->hue(0); 		# root must be manually assigned
    $self->assign_hue; 
    $self->_ps2($fh) if ( exists $attr{colorbox} && $attr{colorbox} );
    $self->_ps($fh,%attr);

    if ( exists $attr{showcount} ) {
	local $maxcount = $attr{showcount};
	print $fh "/Helvetica findfont countsize scalefont setfont\n";
	print $fh "0 setgray\n";
	$self->_ps3($fh);
    }

    print $fh "end\nrestore\n%%EOF\n";
}

1;

__END__

=head1 NAME

Tree - class for manipulating derivation trees

=head1 SYNOPSIS

    use Tree;
    my $tree = Tree->lisplike( '(1 2 (3 4) 5)' );
    print $tree->to_dtree, "\n";

    my %rules = ();
    prepare_rules_for_augment( %rules, <RULEFILE> );
    $tree->augment(%rules);
    print "Original d-tree: ", $tree->to_dtree, "\n";
    print "Nonmonotone d-tree: ", $tree->to_dtree(1), "\n";
    print "PoS-tree: ", $tree->to_ptree, "\n";

    print $tree->graphviz ; # not recommended any more

=head1 DESCRIPTION

I don't think you'll ever need the constructor directly. Use the
C<lisplike> or C<functional> constructors to convert a given Lisp-like
d-tree or functional-notation p-tree string into the internal tree
structure.

=head2 EXPORTED FUNCTIONS

Some functions are related to the C<Tree> class, but do not require an instance
of it to work. 

=over 4

=item C<prepare_rules_for_augment( %hash, @xrs )>

This function prepares a set of XRs rules for use with augmentation of a
d-tree. This function takes a true hash variable as first argument. It
is B<not> legal for this variable to be a tied hash (yet). Any remaining
argument are an XRs rule each. It is legal and supported to use a
filehandle like C<E<lt>FHE<gt>> in place of the second argument.

The return value is the number of rules converted. The hash is indexed
by the rule ID, which is an integer number. A rule ID may be negative,
and may include zero. The value stored in the hash is the following triple: 

=over 4

=item [0]

In the first position is the root of the p-tree representing the LHS. 

=item [1]

In the next position are the I<x> variable indices in the order found on
the RHS, store as an array reference.  The order of variables on the
foreign side is important when re-ordering tree in non-monotone rules.

=item [2]

In the final position is a hash reference with all the rule's
(numerical) attributes, stored as key-value pairs. String values
are currently converted to have their whitespace replaced by C<_>. 

=back

=back 

=head2 METHODS

=over 4

=item C<Tree-E<gt>new>

=item C<Tree-E<gt>new( K1 => V1, .. );>

The regular consructor create a possibly empty tree node. There are a
number of keys that may be passed during the construction of the tree
node.

=over 4

=item C<LABEL>

The I<LABEL> is the rule number associated with this derivation tree
node. The value is an integer. 

=item C<KIDS>

The I<KIDS> array reference points to further tree nodes that are
children of the current node. If absent, the constructor will create
the key, but initialize it with the C<undef> value. 

=item C<POS>

The part-of-speech string is currently unused. The value is a string. 

=back

=item C<Tree-E<gt>lisplike( $string );>

This pseudo-constructor takes a given string, the Lisp-like
representation of a tree, and parses it into a nested C<Tree> structure.
The return value is the parsed tree. Tree nodes are created with a
C<LABEL>.

=item C<Tree-E<gt>functional( $string );>

This pseudo-constructor takes a given string, the function-like
representation of a tree, and parses it into a nested C<Tree> structure.
The return value is the parsed tree. Tree nodes are created with a
C<POS>.

=item C<$self-E<gt>clone ;>

This method is the copy-constructor, creating a new deep copy of the 
current instance. This is useful, if the given subtree is to be inserted
multiple times into another tree. You do not want to insert the original
tree multiple time, but use a clone of it instead. 

=item C<$self-E<gt>label ;>

As getter, this method returns the label of the current tree node. 

=item C<$self-E<gt>label( $new_label );>

As setter, this method sets the label to C<$new_label>. It returns the
previous value of the C<LABEL> field.

=item C<$self-E<gt>kids>

This method returns an array with C<Tree> nodes, which represent the
children of the current node. The returned array may be an empty list,
but it will not be C<undef>.

=item C<$self-E<gt>kids( $child_list_aref );>

=item C<$self-E<gt>kids( @child_list );>

As a setter, this method will replace the current list of children
with the new list of children as passed. This method is usually used
internally. Never pass any children that are not of class C<Tree>.
This method may take either a single array reference to a new list,
or any number of arguments. 

=item C<$self-E<gt>child( $n );>

This method returns the nth child. If there are no children, or the
nth child does not exist, the C<undef> value is returned.

=item C<$self-E<gt>pos ;>

As getter, this method returns the part of speech string of the current
tree node.

=item C<$self-E<gt>pos( $new_pos );>

As setter, this method set the part of speech string of the current
tree node to C<$new_pos>. It returns the old (previous) value fo the
C<POS> field, which may include the C<undef> value. 

=item C<$self-E<gt>to_dtree>

=item C<$self-E<gt>to_dtree( 1 );>

The C<to_dtree> method takes the given tree node C<$self>, and converts
it into a Lisp-like tree representation as string. It will reconstruct
the original d-tree in case of rule-augmented trees. The method uses the
C<LABEL>, C<KIDS> and (optionally) C<ORIG> fields of any tree node. 

If an argument was specified and evaluates to true, the B<new> d-tree
order, as re-ordered by non-monotone rules, will be returned. By
default, it is attempted to always return the original d-tree order. For
non-augmented trees, this does not make any difference. For augmented
tree that didn't employ any I<nonmonotone> rules during augmentation, 
original and new d-trees are identical. 

=item C<$self-E<gt>to_ptree>

The C<to_ptree> method takes the given tree node C<$self>, and converts
it into a functional-style tree representation as string. The method
uses the C<POS> and C<KIDS> fields. It will B<not> work on trees that
have not been rule-augmented yet. 

=item C<$self-E<gt>graphviz>

=item C<$self-E<gt>graphviz( feat => { k => v } ... );>

This method attempts to create a AT&T graphviz input file, suitable for
drawing the tree using C<dot>. It returns the string with the graphviz
instructions.

If used with arguments, it makes the output highly customizable (albeit
not completely deterministic, if rendering options clash). Each argument
is a hash key with an anonymous hash value. The outer hash is indexed by
the rule feature name you want to render rule root nodes
with. Additionally, the following I<special> feature names determine the
default for any node's rendering: 

=over 4

=item __all

This is the default for every node. Any of the other directives just
augments or overwrites settings in I<__all>. By default, it is unset,
but provided as convenience feature. 

=item _root

The I<_root> feature determines the defaults to be used for a rule's 
root node in the p-tree. However, there are no special argument yet
to modify the fact that the PoS is rendered over the rule label.

=item _leaf

The I<_leaf> feature determines the defaults for any leaf in the tree. 
This is usually the lexical string in the target language. Currently,
there is no special rendering for this kind of nodes. 

=item _inner

An I<_inner> node is any node that is neither I<_root> nor I<_leaf>. 
Currently, there is no special rendering for this kind of nodes. 

=back

Populare features to modify nodes with include I<is_lexicalized>,
I<nonmonotone>, I<green>, I<maroon> and I<olive>. You may want to use
features that do not apply to every rule. Support for feature values,
e.g. to determine the strength of any color, is not supported. 

The outer hash key (feature) points to an anonymous hash as its value.
This anonymous hash defines the directives that graphviz understands
when rendering a node, see
L<http://www.graphviz.org/doc/schema/attributes.xml> for
details. Populare directives include I<shape>, I<style>, I<color> and
I<fillcolor>.

Through careful combination, you may be able to visualize multiple
features applying to the same (root) node. However, the case of clashing
features is non-deterministic.

=item C<$self-E<gt>augment( \%prepared_rules );>

This method expands a given (pure) d-tree by the rules passed as
reference in the C<%prepared_rules> hash. Please create this hash using
the L<prepare_rules_for_augment> function.

There are a couple of subtleties. 

=over 4

=item *

The method in-place augments the tree specified by C<$self> with the LHS
of any rules. If you need to remember to original tree, L<clone> it. You
can apply the augment method only once per d-tree root. If you really
need to re-create the original d-tree, you can create the L<to_dtree>
string, and parse it back into a new tree using the L<lisplike>
constructor.

=item *

The rule hash must have at least all rules that are required to translate
the d-tree into a p-tree. You may pass a larger set with more rules, but
not a smaller set. Missing a rule will produce a fatal failure. 

=item *

The function assumes (for now) that the I<nonmonotone> attribute exists
(with arbitrary value) on any rule that re-orders the variables between
LHS and RHS. Only if the I<nonmonotone> attribute is discovered, the 
original d-tree path is remembered. If the I<nonmonotone> attribute is
mistakenly missing from your input rules, you will not be able to recover
the original d-tree path with the L<to_dtree> method.  

=back

=item C<$self-E<gt>nodecount>

This member function does a deep search, and counts the number of all
children and grand-children, etc. It returns the head-count of all 
children. The count includes the node itself, so it is always at least 1.

=item C<$self-E<gt>depth>

The depth in this member is the height of the tree starting in C<$self>.
A depth is the number of edges between the root in C<$self>, and the
lowest-level child.

=item C<$self-E<gt>leaves>

This member returns a list of nodes, looking downwards from the current
node C<$self> for leaf children. The returned list may be empty. 

=item C<$self-E<gt>subroots>

This member function limits the recursion to a given tree in
C<$self>. It returns a list of nodes that are roots of child
sub-trees. The returned list may be empy.

=item C<$self-E<gt>show_assignment( $fh );>

I<This method is a debug function.> It prints, for each level
(y-coordinate) the x coordinates for each node on this level.

=item C<$self-E<gt>assign_hue>

This method recursively assigns a hue in the B<hsb> color space. The
method attempts to use contrastive hues for the same children of a node.

B<Warning:> The root of the full tree must be manually
assigned. Usually, the root node gets the hue 0 like this:

    $root->hue(0);
    $root->assign_hue ;

=item C<$self-E<gt>wide_assign_position>

=item C<$self-E<gt>tight_assign_position>

These two methods (re-) assign a position to each node in the given tree
C<$self>. They follow two different layout
strategies:

=over 4

=item I<wide_assign_position> 

creates wider trees, but all leaves are ordered left to right. You can
pull down the leaves vertically down to a bottom line, and read off the
sentence.

=item I<tight_assign_position>

creates tight trees, where the space between sub-trees is
minimized. This layout is well-suited to print d- and p-trees, but less
suited to trees with alignments.

=back

=item C<$self-E<gt>left>

=item C<$self-E<gt>lower>

=item C<$self-E<gt>right>

=item C<$self-E<gt>upper>

These four method determine the corners of the bounding box of the given
node C<$self>, no recursion. The methods rely that some form of layout
algorithm ran across the tree.

=item C<$self-E<gt>own_bbox>

This method create an array with the supposed bounding box for just the
current node. The four number represent two carthesian coordinates of
the I<lef> I<lower> followed by the I<right> I<upper> corner of the
bounding box for node C<$self> only. The respective methods for the
corners are called as part of assembling the array. No recursion is
done. This method relies that some form of layout algorithm ran across
the tree.

=item C<$self-E<gt>bounding_box>

This method recursively determines the bounding box for the tree
starting in node C<$self>. It returns an array of four numbers,
representing the two carthesian coordinates for the left, lower and
right, upper corner of the bounding box.

=item C<$self-E<gt>level_bounding_box>

This method recursively determines the level-wise bounding boxes for the
tree starting in node C<$self>. It returns an array of arrays of four
numbers, representing the two carthesian coordinates for the left, lower
and right, upper corner of the bounding box for each level. The number
of elements in the outer array represent the depth+1 of the tree.

=item C<$self-E<gt>generate_ps( $fh );>

This method creates an I<Encapsulated PostScript> file (EPS) representing
the layout, and coloring of the tree. 

    $tree->tight_assign_position ;
    $tree->generate_ps( \*STDOUT, colorbox => 1, showcount => 10000 );			

=back 

=head1 EXAMPLES

=head2 D-Tree only

    my $tree = Tree->lisplike( $dtree_string );
    print STDERR "DTREE: ", $tree->to_dtree, "\n";

    $tree->wide_assign_position;
    $tree->show_assignment( \*STDERR ); # optional
    $tree->generate_ps( \*STDOUT );

=head2 P-Tree with count and color

    my $tree = Tree->lisplike( $dtree_string );
    print STDERR "DTREE: ", $tree->to_dtree, "\n";

    my $rulefn = shift;
    open( R, "<$rulefn" ) || die "open $rulefn: $!\n";
    my %rule = ();
    prepare_rules_for_augment( %rule, <R> );
    close R;

    $tree->augment( \%rule );
    print STDERR "PTREE: ", $tree->to_ptree, "\n";

    $tree->tight_assign_position;
    $tree->show_assignment( \*STDERR ); # optional
    $tree->generate_ps( \*STDOUT, colorbox => 1, showcount => 5000 );

=head1 AUTHORS

Jon May, C<jonmay at isi dot edu>,
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

Copyright 1999-2008 The University of Southern California. All rights
reserved.

=cut
