#
# class to encapsulate a DAG (lattice). 
#
# The resulting data structure is as follows:
#
#  { NODES => [
#    { IN => [ FLATEDGE# ]
#      OUT => [ FLATEDGE# ]
#      LABEL => $
#      FEATS => %
#    } ]
#    FLATEDGE => [ # all edges, even from inside blocks
#    { INDEX => $
#      START => $
#      FINAL => $
#      WHERE => $ # string address of real edge
#      POINTER => \% # ref to real edge
#    EDGES => [ # top-level edges
#    { INDEX => $ # relative
#      START => $ # start node
#      FINAL => $ # end node
#      LABEL => $
#      FEATS => %
#    }
#    BLOCKS => [ 
#    { INDEX => $ # relative
#      BLOCKS => [ ... ]
#      EDGES  => [ ... ]
#      FEATS  => %
#    }
#  }
#
# The flat edges use a string address where any number before any colon is a
# block number, and the final number (or only number for top-level edges) is
# the edge inside that block: [.. : [b# : [b# : ]]] e#
# Alternatively, a pointer straight to the edge is provided, too. 
#
package Lattice::Graph;
use v5.8.8;
use strict;

use Lattice::Base;
our @ISA = qw(Lattice::Base);

our @EXPORT_OK = ();
our %EXPORT_TAGS = ( 'all' => [ @EXPORT_OK ] );
our @EXPORT = ();

our $VERSION = '1.0';
$VERSION = $1 if ( '$Revision$' =~ /Revision:\s+([0-9.]+)/ );

use Carp;
use Lattice::Node;
use Lattice::Edge;
use Lattice::Block;

sub new {
    my $proto = shift;
    my $class = $proto || ref $proto || __PACKAGE__;

    bless Lattice::Base->new( NODES => [], 
			      EDGES => [], 
			      BLOCKS => [], 
			      FLATEDGE => [], 
			      @_ ), $class;
}

sub add_node {
    my $self = shift;

    my $node;
    if ( ref $_[0] eq 'Lattice::Node' ) {
	# passing a Lattice::Node, fine
	$node = shift;
    } elsif ( ! ref $_[0] && ( (@_+0) & 1 ) == 0 ) {
	# passing lose arguments as list hash
	$node = Lattice::Node->new(@_);
    } else {
	# dunno what you passed
	confess "Argument(s) is neither a node nor node-specification";
    }
    
    $self->{NODES}->[ $node->index ] = $node;
}

sub add_edge {
    my $self = shift;

    my $edge;
    if ( ref $_[0] eq 'Lattice::Edge' ) {
	# passing a Lattice::Edge, fine
	$edge = shift;
    } elsif ( ! ref $_[0] && ( (@_+0) & 1 ) == 0 ) {
	# passing lose arguments as list hash
	$edge = Lattice::Edge->new(@_);
    } else {
	# dunno what you passed
	confess "Argument(s) is neither an edge nor edge-specification";
    }

    my $e = @{$self->{EDGES}} + 0;
    $edge->index( $e );		# overwrite -- OUR count counts
    push( @{$self->{EDGES}}, $edge );

    my $f = @{$self->{FLATEDGE}} + 0;
    push( @{$self->{FLATEDGE}}, { INDEX => $f, 
				  START => $edge->start(),
				  FINAL => $edge->final(),
				  WHERE => $edge->index(), # outer-most edge
				  POINTER => $self->{EDGES}->[$e] } );

    # add flatedge to nodes (create node, unless it exists)
    $self->outgoing( $edge->start(), $f );
    $self->incoming( $edge->final(), $f );
}

sub outgoing {
    # purpose: add out-going flatedge IDs to a given node ID
    # paramtr: $nodeid (IN): node number
    #          @flatedge (IN): any number of flatedge IDs
    # 
    my $self = shift;
    my $node = shift;
    $self->add_node( INDEX => $node ) 
	unless defined $self->{NODES}->[$node];
    $self->{NODES}->[$node]->outgoing(@_);
}

sub incoming {
    # purpose: add incoming flatedge IDs to a given node ID
    # paramtr: $nodeid (IN): node number
    #          @flatedge (IN): any number of flatedge IDs
    # 
    my $self = shift;
    my $node = shift;
    $self->add_node( INDEX => $node ) 
	unless defined $self->{NODES}->[$node];
    $self->{NODES}->[$node]->incoming(@_);
}

sub flatten_edge_block {
    # purpose: recursively gather edges hidden inside blocks
    # paramtr: $item (IN): Lattice::{Edge,Block} instance
    #          @addr (IN): symbolic addressing using block/edge indices
    # warning: internal use!
    #
    my $self = shift;
    my $item = shift;
    my $p_block = $item->isa('Lattice::Block'); # cache
    my $p_edge  = $item->isa('Lattice::Edge');	# cache
    croak( "Illegal argument type ", ref $item, " for $item" )
	unless ( $p_block || $p_edge );
    my @addr = ( @_, $item->index() );

    if ( $p_block ) {
	foreach my $j ( ( @{ $item->edges() }, @{ $item->blocks() } ) ) {
	    $self->flatten_edge_block( $j, @addr );
	}
    } elsif ( $p_edge ) {
	my $n = @{$self->{FLATEDGE}} + 0;
	push( @{$self->{FLATEDGE}}, { INDEX => $n,
				      START => $item->start(),
				      FINAL => $item->final(),
				      WHERE => join(':',@addr),
				      POINTER => $item } );

	# add flatedge to nodes (create node, unless it exists)
	$self->outgoing( $item->start(), $n );
	$self->incoming( $item->final(), $n );
    } else {
	croak "Illegal argument $item";
    }
}

sub add_block {
    my $self = shift;

    my $block;
    if ( ref $_[0] eq 'Lattice::Block' ) {
	# passing a Lattice::Edge, fine
	$block = shift;
    } elsif ( ! ref $_[0] && ( (@_+0) & 1 ) == 0 ) {
	# passing lose arguments as list hash
	$block = Lattice::Block->new(@_);
    } else {
	# dunno what you passed
	confess "Argument(s) is neither a block nor block-specification";
    }

    my $b = @{$self->{BLOCKS}} + 0;
    $block->index($b);		# overwrite -- OUR count counts
    push( @{$self->{BLOCKS}}, $block );
    $self->flatten_edge_block( $block );
}

sub add_item {
    # purpose: add any number of nodes, edges or blocks, even mixed. 
    # paramtr: One or more Lattice::(Edge|Node|Block) in any order
    #
    my $self = shift;
    confess "I need argument(s)" unless @_;

    foreach my $item ( @_ ) {
	if ( $item->isa('Lattice::Edge') ) {
	    $self->add_edge($item);
	} elsif ( $item->isa('Lattice::Node') ) {
	    $self->add_node($item);
	} elsif ( $item->isa('Lattice::Block') ) {
	    $self->add_block($item);
	} else {
	    confess( 'Cannot add instance of ', ref $item, ' to DAG.' );
	}
    }
}

sub delete_edge_by_text {
    # purpose: remove a given edge from a graph
    # warning: some portion of some arrays will have undef 
    # paramtr: $text (IN): LABEL of the edge to remove
    #
    my $self = shift;
    my $text = shift;

    for ( my $i=0; $i < @{$self->{FLATEDGE}}; ++$i ) {
	next unless defined $self->{FLATEDGE}->[$i];
	next unless exists $self->{FLATEDGE}->[$i]->{POINTER}->{LABEL} &&
	    $self->{FLATEDGE}->[$i]->{POINTER}->{LABEL} eq $text;
	# found an edge with the start symbol at flatedge# $i
	    
	# 1. remove all such edges -- find path to inside edge
	my @a = split( /:/, $self->{FLATEDGE}->[$i]->{WHERE} ); 
	my $e = pop(@a);	# last element is always edge index
	my $p = $self;		# start at DAG base
	foreach my $j ( @a ) {	# may not be executed at all
	    confess "Something is odd!" unless defined $p->{BLOCKS}->[$j];
	    $p = $p->{BLOCKS}->[$j];
	}
	delete $p->{EDGES}->[$e];
	$self->{FLATEDGE}->[$i] = undef;

	# 2. remove edge from node lists
	foreach my $n ( @{$self->{NODES}} ) {
	    @{$n->{IN}} = grep { $_ != $i } @{$n->{IN}};
	    @{$n->{OUT}} = grep { $_ != $i } @{$n->{OUT}}; 
	    # remove node, if no flatedges point to it
	    if ( @{$n->{IN}} + @{$n->{OUT}} == 0 ) {
		warn( "Warning: node $n was explicitly set, but has ",
		      "no more edges connecting it, removing\n" )
		    if exists $n->{explicit};
		$n = undef;
	    }
	}
    }
}

sub c_escape($) {
    my $s = shift;
    $s =~ s{([\\""])}{\\$1}g;
    $s;
}

sub graphviz {
    # purpose: generate output string proper as graphviz input
    # paramtr: %a (opt. arguments overwriting defaults)
    # returns: string
    #
    my $self = shift;
    my %attr = ( nodeshape => 'circle'
	       , nodefont => 'Helvetica'
	       , nodesize => 8.0
	       , edgefont => 'Helvetica'
	       , edgesize => 10.0
	       , @_ # MUST BE LAST
	       );

    # start header
    my $s = 'strict digraph ';
    $s .= "\"$attr{title}\" " if exists $attr{title};
    $s .= "{\n";
    $s .= "  node [shape=$attr{nodeshape}";
    $s .= ",fontname=$attr{nodefont},fontsize=$attr{nodesize}]\n";
    $s .= "  edge [arrowhead=normal,arrowsize=1.0";
    $s .= ",fontname=$attr{edgefont},fontsize=$attr{edgesize}]\n";

    # list of nodes
    for ( my $n=0; $n < @{$self->{NODES}}; ++$n ) {
	my $node = $self->{NODES}->[$n];
	next unless defined $node;
	$s .= "  $n [label=\"$n\"]\n";
    }

    # list of edges
    for ( my $e=0; $e < @{$self->{FLATEDGE}}; ++$e ) {
	my $edge = $self->{FLATEDGE}->[$e];
	next unless defined $edge;
	$s .= "  " . $edge->{START} . ' -> ' . $edge->{FINAL} . ' [';
	my $text = c_escape($edge->{POINTER}->{LABEL}) || '';
	$s .= "label=\"$text\""; #  if length($text);
	$s .= ",color=blue" 
	    if ( exists $edge->{POINTER}->{FEATS} &&
		 keys %{$edge->{POINTER}->{FEATS}} > 0 );
	$s .= "]\n";
    }

    # trailer
    $s .= "}\n";
    $s;
}

sub show {
    my $self = shift;
    my %attr = ( @_ );

    my $result = 'lattice';
    if ( exists $self->{FEATS} ) {
	my ($k,$v);
	while ( ($k,$v) = each %{$self->{FEATS}} ) {
	    $result .= " $k=\"" . c_escape($v) . "\""; 
	}
    }
    $result .= ' {';
    $result .= ( exists $attr{minimal} ? ' ' : "\n" );

    # order output by node# 
    foreach my $i ( # step 3: collapse back to itemref
		    map { $_->[0] }

		    # step 2: sort by node#
		    sort { $a->[1] <=> $b->[1] }

		    # step 1: construct array from all (nodes,edges,blocks)
		    #         each item is an array of [ itemref, node# ]
		    ( ( map { [ $_, $_->index() ] }
			grep { defined $_ } @{$self->{NODES}} )
		    , ( map { [ $_, $_->start() ] } 
			grep { defined $_ } @{$self->{EDGES}} )
		    , ( map { [ $_, $_->start() ] } 
			grep { defined $_ } @{$self->{BLOCKS}} )
		    ) 
		  ) {
	$result .= $i->show(%attr);
    }

    $result .= '};';
    $result .= "\n" unless exists $attr{minimal};
    $result;
}

sub normalize {
    # purpose: renumber nodes to start at 0, increment 1
    # warning: this will create a new NODES and FLATEDGE internally
    # warning: this will not compact the EDGES nor BLOCKS in any way
    # paramtr: -
    #
    my $self = shift;

    # remember nodes that have flatedges between them
    # incidentally, this should be all nodes with edges
    my %seen = ();
    foreach my $f ( grep { defined $_ } @{$self->{FLATEDGE}} ) {
	$seen{ $f->{START} }=1;
	$seen{ $f->{FINAL} }=1;
    }

    # normalize node numbering -- edge-linked nodes only!
    my %norm = ();
    my $count = 0;
    for ( my $n=0; $n < @{$self->{NODES}}; ++$n ) {
	next unless exists $seen{$n}; 
	$norm{$n} = $count++;
    }

    # normalize flatedge numbering
    my (@flatedge,@incoming,@outgoing);
    my $flatedge = 0;
    foreach my $f ( grep { defined $_ } @{$self->{FLATEDGE}} ) {
	# create pseudo-(table)-edge with corrected numbering
	my $start = $norm{ $f->{START} };
	my $final = $norm{ $f->{FINAL} };
	push( @flatedge, { START => $start
			 , FINAL => $final
			 , INDEX => $flatedge
			 , WHERE => $f->{WHERE}
			 , POINTER => $f->{POINTER} 
		         } );

	# update true EDGE endpoints
	$f->{POINTER}->{START} = $start;
	$f->{POINTER}->{FINAL} = $final;

	# remember list of in/out for nodes
	push( @{$incoming[$final]}, $flatedge );
	push( @{$outgoing[$start]}, $flatedge );

	$flatedge++;
    }

    # replace flatedge
    delete $self->{FLATEDGE}; 
    $self->{FLATEDGE} = \@flatedge;

    # update all FLATEDGE in in/out of NODES
    my @nodes = ();
    for ( my $n=0; $n < @{$self->{NODES}}; ++$n ) {
	next unless exists $seen{$n}; 
	push( @nodes, 
	      $self->{NODES}->[$n]->clone( INDEX => $norm{$n}
					 , IN  => $incoming[$norm{$n}] || []
					 , OUT => $outgoing[$norm{$n}] || []
					 ) );
    }

    delete $self->{NODES};
    $self->{NODES} = [];
    foreach my $node ( @nodes ) {
	$self->{NODES}->[ $node->index ] = $node; 
    }
}


sub max(@) {
    my $max = shift;
    foreach my $x ( @_ ) {
	$max = $x if $x > $max;
    }
    $max;
}

sub table {
    # purpose: create 2D table for display of lattice
    # returns: { width => $ # maximum column width
    #            table => [ { row => $
    #                       , bad => $
    #                       , col => [ FINAL => $, 
    #                                  LABEL => $,
    #                                  FEATS => % ] 
    #                       } ] }
    #
    my $self = shift;

    # remember nodes that have flatedges between them
    my %seen = ();
    foreach my $f ( grep { defined $_ } @{$self->{FLATEDGE}} ) {
	$seen{ $f->{START} }=1;
	$seen{ $f->{FINAL} }=1;
    }

    # normalize node numbering -- edge-linked nodes only!
    my %norm = ();
    my $count = 0;
    for ( my $n=0; $n < @{$self->{NODES}}; ++$n ) {
	next unless exists $seen{$n};
	$norm{$n} = $count++;
    }

    # normalize flatedge numbering, and construct table
    my @table = map { { row => $_, bad => 0, col => [] } } 0 .. $count-2;
    my @spans = ();
    foreach my $f ( grep { defined $_ } @{$self->{FLATEDGE}} ) {
	# create pseudo-(table)-edge with corrected numbering
	my $start = $norm{ $f->{START} };
	my $final = $norm{ $f->{FINAL} };
	my $edge = { START => $start,
		     FINAL => $final,
		     LABEL => $f->{POINTER}->{LABEL} || '',
		     FEATS => $f->{POINTER}->{FEATS} || {} };

	# mark edge in table
	my $row = 0;
	for (;; ++$row ) {
	    # check for overlaps
	    my $seen = 0;
	    for ( my $i=$start; $i<$final; ++$i ) {
		$seen++ if defined $spans[$i][$row]; 
	    }
	    last unless $seen;
	}
	$table[$start]{col}[$row] = $edge;

	# multi-column spans are bad split points
	for ( my $i=$start; $i<$final; ++$i ) {
	    $spans[$i][$row] = 1; # mark table position as used
	    if ( $i == $start ) {
		$table[$i]{col}[$row] = $edge;
	    } else {
		$table[$i]{bad}++;
	    }
	}
    }

    { width => max( map { defined $_ ? scalar @{$_->{col}} : 0 } @table ),
      table => [ grep { defined $_ } @table ] }; 
}


1;

__END__

=head1 NAME

Lattice::Graph - Class to encapsulate a single lattice (DAG). 

=head1 SYNOPSIS

    use Lattice::Graph;

=head1 DESCRIPTION

Please use L<Lattice::Tool> for simplified access to the lattice parser!

=head2 METHODS

=over 4

=item C<Lattice::Graph-E<gt>new;>

The constructor creates the internal data structure to remember all
nodes, blocks, edges, and flattened edges.

The optional argument is they key C<FEATS> with an appropriate hash
reference value of feature entries.

=item C<$self-E<gt>add_node>

This method takes an argument of L<Lattice::Node> to add the given node
to the list of vertices. In an alternative invocation, you may pass all
the arguments you would pass to the constructor of L<Lattice::Node>, to
construct an instance on the fly.

=item C<$self-E<gt>add_edge>

This method takes an argument of L<Lattice::Edge> to add the given edge
to the list of edges. In an alternative invocation, you may pass all the
arguments you would pass to the constructor of L<Lattice::Edge>, to
construct an instance on the fly.

Adding an edge will update the list of flat edges. It will also add the
flatedge's start point to the node's outgoing list, and the flatedge's
final point to the node's incoming list.

=item C<$self-E<gt>outgoing( $node, $f1, .. );>

This method add one or more flatedge indices to the list of outgoing
flatedges of the given C<$node>. The node is auto-vivified, if it does
not exist.

=item C<$self-E<gt>incoming( $node, $f1, .. );>

This method add one or more flatedge indices to the list of outgoing
flatedges of the given C<$node>. The node is auto-vivified, if it does
not exist.

=item C<$self-E<gt>flatten_edge_block>

This internal helper method recursively gathers all edges hidden inside
a block into the list of flatedges. The node's outgoing and incoming
edge list are also updated as part of the gathering.

=item C<$self-E<gt>add_block>

This method takes an argument of L<Lattice::Block> to add the given
block to the list of blocks. In an alternative invocation, you may pass
all the arguments you would pass to the constructor of L<Lattice::Block>,
to construct an instance on the fly.

Adding a block will also update the list of flat edges by calling the
L<flatten_edge_block> method on the graph instance. 

=item C<$self-E<gt>add_item( $item, .. );>

This convenience method permits to add any edge, block or node
constituent to a graph. The method takes one or more arguments, and any
mixture of, L<Lattice::Edge>, L<Lattice::Block> and L<Lattice::Node>
instances.

=item C<$self-E<gt>delete_edge_by_text($label);>

This method searches the graph for all edges that have a label which
matches the argument. If found, the given edge, however deep nested
inside a block, is removed. Its corresponding flat edge is also removed.
However, array positions are not collapsed - they will retain an
C<undef> value.

The method does not have a sensible return value. It may or may not have
removed anything.

=item C<$self-E<gt>graphviz;>

=item C<$self-E<gt>graphviz( $k1 => $v1, ... );>

This helper method creates output for the graphviz graph drawer. It may
take an arbitrary (even) number of arguments overriding output formating
for graphviz. 

The possible keys and values are to be explained here later.

=item C<$self-E<gt>show;>

=item C<$self-E<gt>show( KEY => $value, ... );>

This method creates a string representation of the current DAG as single
string result. It tries to order output. It does B<not> transform
feature values like C<new_decoder_weight_format>. However, it does try
to be conservative producing the DAG.

=over 4

=item B<indent>

This key represents the indentation string. Usually, you won't need to
set it. It is passed internally when recursing into blocks, and defaults
to two spaces.

=item B<minimal>

If this key is present, the lattice is formatted without any line
separators (except for those present inside labels or features).
Usually, these are one-liners. Despite the key name, a significant
number of spaces is retained to make it halfway legible. 

=back

=item C<$self-E<gt>table>

This method creates a data structure to facilitate the tabular formating
of a given lattice. It tries to avoid crossing edges by adding columns
until it is possible to print a non-edge crossing. It returns a hash
reference with the two top-level keys. As part of creating the table,
the node and edge numbers are normalized I<without> changing the
underlying graph object (so you will only see it in the table). 

The key C<width> contains the column count, how deep the internal
structures may stretch to accomodate. The C<table> key contains the
content mappings. The number of entries in the table is also the height
of the table. You are free to use the table in transposed form, swapping
rows and columns. Visualization does that. 

Given a simple lattice

    lattice {
      [0,1] "A";
      [1,2] "B";
      [2,3] "C";
      [0,3] "ABC";
    };

you obtain the table 

    $t = { 'width' => 2,
           'table' => [ { 'bad' => 0,
                          'col' => [ {'FINAL' => 1,
                                      'START' => 0,
                                      'FEATS' => {},
                                      'LABEL' => 'A'},
                                     {'FINAL' => 3,
                                      'START' => 0,
                                      'FEATS' => {},
                                      'LABEL' => 'ABC'} ],
                          'row' => 0 },
                        { 'bad' => 1,
                          'col' => [ {'FINAL' => 2,
                                      'START' => 1,
                                      'FEATS' => {},
                                      'LABEL' => 'B'} ],
                          'row' => 1 },
                        { 'bad' => 1,
                          'col' => [ {'FINAL' => 3,
                                      'START' => 2,
                                      'FEATS' => {},
                                      'LABEL' => 'C'}],
                          'row' => 2 } ] };

The C<table> array has 3 entries, as many as there are distinct minimal
edges.

=over 4

=item B<row>

This field repeats the current row. It is useful, if you are traversing
the table with C<foreach> iterators instead of an indexed C<for> loop.

=item B<bad>

This entry is non-zero, if the current row is a bad split point. This is
useful, if your table is too large, and you want to split output across
multiple tables, presumably on multiple pages.

=item B<col>

This entry is an array of edge-like hashes. The array may contain
C<undef> entries, e.g. in the above table, you would expect each C<col>
table to be 2 entries deep. However, the second entry of the second and
third table is C<undef>. A table may contain explicit C<undef> values,
if it is not the final one.

=back 

=back

=head2 INHERITED

=over 4

=item C<$self-E<gt>feature>

=item C<$self-E<gt>features>

This class inherits above methods from L<Lattice::Base> to manage the
features.

=back

=head1 SEE ALSO

L<Lattice::Base>, L<Lattice::Edge>, L<Lattice::Node>, L<Lattice::Block>.

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

Copyright 1999-2008 The University of Southern California. All rights
reserved.

=cut
