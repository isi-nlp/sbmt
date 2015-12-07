#
# class to encapsulate graph blocks
#
package Lattice::Block;
use v5.8.8;
use strict;

use Lattice::Base;
use Lattice::Edge;
require Exporter;
our @ISA = qw(Lattice::Base);


our @EXPORT_OK = qw();
our %EXPORT_TAGS = ( 'all' => [ @EXPORT_OK ] );
our @EXPORT = qw();

our $VERSION = '1.0';
$VERSION = $1 if ( '$Revision$' =~ /Revision:\s+([0-9.]+)/ );

use Carp;

sub new {
    my $proto = shift;
    my $class = $proto || ref $proto || __PACKAGE__;

    # no sanity checks for blocks
    bless Lattice::Base->new( BLOCKS => [], EDGES => [], @_ ), $class;
}

sub index {
    my $self = shift;
    my $result = $self->{INDEX};
    $self->{INDEX} = shift if @_;
    $result;
}

sub add_edge {
    # purpose: Append sub-edge to list of edges
    # paramtr: $edge (IN): sub-edge to add
    # returns: edge index
    #
    my $self = shift;
    my $edge = shift;
    croak "This is not an edge" unless ref $edge eq 'Lattice::Edge';

    my $n = @{$self->{EDGES}};
    push( @{$self->{EDGES}}, $edge );
    $edge->index($n);		# adjust index
}

sub add_block {
    # purpose: Append sub-block to list of blocks
    # paramtr: $block (IN): sub-block to add
    # returns: block index
    #
    my $self = shift;
    my $block = shift;
    croak "This is not a block" unless ref $block eq 'Lattice::Block';

    my $n = @{$self->{BLOCKS}};
    push( @{$self->{BLOCKS}}, $block );
    $block->index($n);		# adjust index
}

sub add_item {
    # purpose: add any number of edges or blocks, even mixed. 
    # paramtr: One or more Lattice::(Edge|Block) in any order
    #
    my $self = shift;
    confess "I need argument(s)" unless @_;

    foreach my $item ( @_ ) {
	if ( $item->isa('Lattice::Edge') ) {
	    $self->add_edge($item);
	} elsif ( $item->isa('Lattice::Block') ) {
	    $self->add_block($item);
	} else {
	    confess( 'Cannot add instance of ', ref $item, ' to block.' );
	}
    }
}

sub edge {
    my $self = shift;
    my $index = shift;
    croak "Edge index argument is required" unless defined $index;
    croak "$index is not a valid edge index" 
	unless ( $index > 0 && $index < @{$self->{EDGES}} );

    my $result = $self->{EDGES}->[$index];
    if ( @_ ) {
	my $edge = shift;
	if ( ref $edge eq 'Lattice::Edge' ) {
	    $self->{EDGES}->[$index] = $edge;
	} else {
	    croak "You must use a Lattice::Edge in the setter";
	}
    }
    $result;
}

sub block {
    my $self = shift;
    my $index = shift;
    croak "Block index argument is required" unless defined $index;
    croak "$index is not a valid block index" 
	unless ( $index > 0 && $index < @{$self->{BLOCKS}} );

    my $result = $self->{BLOCKS}->[$index];
    if ( @_ ) {
	my $block = shift;
	if ( ref $block eq 'Lattice::Block' ) {
	    $self->{BLOCKS}->[$index] = $block;
	} else {
	    croak "You must use a Lattice::Block in the setter";
	}
    }
    $result;
}

sub min {
    my $min = shift;
    foreach my $x ( @_ ) {
	$min = $x if $x < $min;
    }
    $min;
}

sub start {
    # purpose: find in all edges the one with with the smallest start
    # returns: node number, or -1 if there are no edges, internal or other
    #
    my $self = shift;
    
    my @block = map { $_->start() } grep { defined $_ } @{$self->{BLOCKS}};
    my @edge  = map { $_->start() } grep { defined $_ } @{$self->{EDGES}};

    # if both arrays are empty, return -1
    ( @edge + @block == 0 ? -1 : min(@edge,@block) );
}

sub show {
    #
    my $self = shift;
    my %attr = ( @_ );
    my $indent = $attr{indent} || ( exists $attr{minimal} ? '' : '  ' );
    my $result = $indent . 'block';

    if ( exists $self->{FEATS} ) {
	my ($k,$v);
	while ( ($k,$v) = each %{$self->{FEATS}} ) {
	    $result .= " $k=\"" . Lattice::Graph::c_escape($v) . "\""; 
	}
    }
    $result .= ' {';
    $result .= ( exists $attr{minimal} ? ' ' : "\n" );

    # order output by node# 
    $attr{indent} = substr( $indent, -2 ) . $indent;
    foreach my $i ( # step 3: collapse back to itemref
		    map { $_->[0] }

		    # step 2: sort by start#
		    sort { $a->[1] <=> $b->[1] }

		    # step 1: construct array from all (edges,blocks)
		    #         each item is an array of [ itemref, start# ]
		    map { [ $_, $_->start() ] } 
		    grep { defined $_ } 
		    ( @{$self->{EDGES}}, @{$self->{BLOCKS}} )
		  ) {
	$result .= $i->show(%attr);
    }

    $result .= "$indent};";
    $result .= ( exists $attr{minimal} ? ' ' : "\n" );
    $result;
}

1;

__END__


=head1 NAME

Lattice::Block - Class to encapsulate blocks. Blocks may contain other
blocks or edges, so they are a limited sub-graph.

=head1 SYNOPSIS

    use Lattice::Block;

=head1 DESCRIPTION

=head2 METHODS

=over 4

=item C<Lattice::Block-E<gt>new >

The constructor initialized an empty list of blocks and empty list of
edges. It does not require any special arguments to create a block. 

=item C<$self-E<gt>index>

=item C<$self-E<gt>index( $new_index );>

A block may have a notion of an index. Dunno how useful that is yet. As
usual, B<you must not change the index, after a block was added to any
graph>. The I<setter> is used by L<Lattice::Graph> to set its internal
block index.

=item C<$self-E<gt>add_edge( $edge );>

This method takes a given edge, and add it to this block. As part of the
block addition, the edge's index will be updated to match its position
within the block.

=item C<$self-E<gt>add_block( $block );>

This method takes a readily assembled block, and adds it to this block.
As part of the block addition, the added block's index will be updated
to match its position within this block.

=item C<$self-E<gt>add_item( $item);>

This convenience method permits to add any edge or block constituent to
this block. The method takes one or more arguments, and any mixture of,
L<Lattice::Edge> and L<Lattice::Block> instances.

=item C<$self-E<gt>edge( $n );>

As I<getter>, this method returns the C<$n>th internal edge. C<undef> is
returned for non-existing edges.

=item C<$self-E<gt>edge( $n, $new_edge );>

As I<setter>, this method creates or replaces the C<$n>th internal edge
with the new edge.

=item C<$self-E<gt>block( $n );>

As I<getter>, this method returns the C<$n>th sub-block. C<undef> is
returned for non-existing sub-blocks.

=item C<$self-E<gt>block( $n, $new_block );>

As I<setter>, this method creates or replaces the C<$n>th internal
sub-block with the new block.

=item C<$self-E<gt>start>

This pseudo-method recursively determines the lowest start point of any
contained edge. Returns -1, if this block does not contain any edges. 

=item C<$self-E<gt>show;>

=item C<$self-E<gt>show( KEY => $value, ... );>

This method creates a string representation of the current block as
single string result. It tries to order output, and recursively
traverses all sub-blocks.

=back

=head2 INHERITED

=over 4

=item C<$self-E<gt>feature>

=item C<$self-E<gt>features>

This class inherits above methods from L<Lattice::Base> to manage the
features.

=back

=head1 SEE ALSO

L<Lattice::Base>, L<Lattice::Edge>, L<Lattice::Graph>.

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
