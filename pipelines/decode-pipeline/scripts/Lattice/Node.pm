#
# class to encapsulate graph nodes
#
package Lattice::Node;
use v5.8.8;
use strict;

use Lattice::Base;
require Exporter;
our @ISA = qw(Lattice::Base);
our $default_label = ' ';

our @EXPORT_OK = qw( $default_label );
our %EXPORT_TAGS = ( 'all' => [ @EXPORT_OK ] );
our @EXPORT = qw();

our $VERSION = '1.0';
$VERSION = $1 if ( '$Revision$' =~ /Revision:\s+([0-9.]+)/ );

use Carp;

sub new {
    my $proto = shift;
    my $class = $proto || ref $proto || __PACKAGE__;

    my %attr = ( @_ );
    croak "Any node must have an index" unless exists $attr{INDEX};
    croak "A node index is a non-negative integer" unless $attr{INDEX} >= 0;
    push( @_, 'explicit', 1 ) if exists $attr{LABEL};

    bless Lattice::Base->new( LABEL => $default_label
			    , IN => []
			    , OUT => []
			    , @_ #  MUST BE LAST
			    ), $class;
}

sub clone {
    my $self = shift;
    bless Lattice::Base->new( %{$self}, @_ ), ref $self;
}

sub label {
    my $self = shift;
    # avoid auto-vivification
    my $result = exists $self->{LABEL} ? $self->{LABEL} : undef;
    if ( @_ ) {
	$self->{LABEL} = shift;
	$self->{explicit} = 1;
    }
    $result;
}

sub explicit {
    # warning: this is a read-only accessor (getter only)
    # returns: true, if the edge was explicit, false if implicit
    my $self = shift;
    exists $self->{explicit};
}

sub index {
    my $self = shift;
    $self->{INDEX};
}

sub outgoing {
    # purpose: manage outgoing flatedge indices
    # paramtr: one or more numerical indices of a flatedge
    # returns: new state of outgoing flatedges
    #
    my $self = shift;
    @{$self->{OUT}} = keys %{{ map { $_ => 1 } ( @{$self->{OUT}}, @_ ) }};
}

sub incoming {
    # purpose: manage incoming flatedge indices
    # paramtr: one or more numerical indices of a flatedge
    # returns: new state of incoming flatedges
    #
    my $self = shift;
    @{$self->{IN}} = keys %{{ map { $_ => 1 } ( @{$self->{IN}}, @_ ) }};
}

sub show {
    #
    my $self = shift;
    return '' unless exists $self->{explicit};

    my %attr = ( @_ );
    my $result = $attr{indent} || ( exists $attr{minimal} ? '' : '  ' );
    $result .= '[' . ( $self->{INDEX} || $attr{index} ) . ']';

    # FIXME: This will print the default label :-(
    if ( exists $self->{LABEL} ) {
	my $text = Lattice::Graph::c_escape($self->{LABEL});
	$result .= " \"$text\"";
    }

    if ( exists $self->{FEATS} ) {
	my ($k,$v);
	while ( ($k,$v) = each %{$self->{FEATS}} ) {
	    $result .= " $k=\"" . Lattice::Graph::c_escape($v) . "\""; 
	}
    }
    $result .= ';';
    $result .= ( exists $attr{minimal} ? ' ' : "\n" );
    $result;
}

1;

__END__

=head1 NAME

Lattice::Node - Class to encapsulate explicit and implicit vertices. 

=head1 SYNOPSIS

    use Lattice::Node;

=head1 DESCRIPTION

=head2 METHODS

=over 4

=item C<Lattice::Node-E<gt>new( INDEX => nr )>
=item C<Lattice::Node-E<gt>new( INDEX => nr, ... )>

The constructor create a new vertex instance to be added to the graph
later. The mandatory argument is the node index, a non-negative integer
introduced by its key C<INDEX>.

If a C<LABEL> is provided, the node will be marked as explicitly
constructed, even if the label value corresponds to the default label.
Explicit nodes are printed in the graph output. 

Without C<LABEL>, the node is marked implicit, and will not be printed
as part of the graph printing. The internal label is assigned with the
default label.

=item C<$self-E<gt>label>

As I<getter>, this method returns the node's label. For implicit nodes,
this will be the default label.

=item C<$self-E<gt>label( $new_label );>

As I<setter>, this method sets a new label on a node. Furthermore, the
node will be marked as I<explicit> for purpose of printing. The return
value is the previous label value.

=item C<$self-E<gt>explicit;>

This method is a I<getter> only. It returns a true value, if the node
is explicit, and a false value, if it is implicit. 

=item C<$self-E<gt>index;>

As I<getter>, this method returns the node's notion of its own index.
You must not attempt to change the index except through construction. 

=item C<$self-E<gt>outgoing( $i1, ... );>

This method adds one or more flatedge indices to the list of outgoing
flat edges. The method maintains the indices as list, removing
duplicates as part of adding flatedge indices.

=item C<$self-E<gt>incoming( $i1, ... );>

This method adds one or more flatedge indices to the list of incoming
flat edges. The method maintains the indices as list, removing
duplicates as part of adding flatedge indices.


=item C<$self-E<gt>show;>

=item C<$self-E<gt>show( KEY => $value, ... );>

This method returns a string representing the node. Implicit nodes cause
an empty string. Label and feature values are C-escaped strings.

=back

=head2 INHERITED

=over 4

=item C<$self-E<gt>feature>

=item C<$self-E<gt>features>

This class inherits above methods from L<Lattice::Base> to manage the
features.

=back

=head1 SEE ALSO

L<Lattice::Base>, L<Lattice::Graph>.

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
