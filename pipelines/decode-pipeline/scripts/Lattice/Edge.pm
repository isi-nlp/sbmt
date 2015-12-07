#
# class to encapsulate graph edges
#
package Lattice::Edge;
use v5.8.8;
use strict;

use Lattice::Base;
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

    my %attr = ( @_ );
    croak "Any edge must have a beginning" unless exists $attr{START};
    croak "An edge start is a non-negative integer" unless $attr{START} >= 0;
    croak "Any edge must have an ending" unless exists $attr{FINAL};
    croak "An edge end is a non-negative integer" unless $attr{FINAL} >= 0;
    croak "Acyclacy enforcement failure: Edge start after it ends"
	unless $attr{START} < $attr{FINAL}; 

    bless Lattice::Base->new( %attr ), $class;
}

sub label {
    my $self = shift;
    my $result = $self->{LABEL};
    $self->{LABEL} = shift if ( @_ );
    $result;
}

sub start {
    my $self = shift;
    $self->{START};
}

sub final {
    my $self = shift;
    $self->{FINAL};
}

sub show {
    #
    my $self = shift;
    my %attr = ( @_ );
    my $result = $attr{indent} || ( exists $attr{minimal} ? '' : '  ' );

    $result .= '[' . $self->{START} . ',' . $self->{FINAL} . ']';

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

Lattice::Edge - Class to encapsulate edges. 

=head1 SYNOPSIS

    use Lattice::Edge;

=head1 DESCRIPTION

=head2 METHODS

=over 4

=item C<Lattice::Edge-E<gt>new( START => nr, FINAL => nr );>

=item C<Lattice::Edge-E<gt>new( START => nr, FINAL => nr, ... );>

The constructor create a new instance of an edge. An edge must provide a
C<START> and a C<FINAL> point of the edge, as non-negative integer.
Furthermore, the constructor enforces that the C<FINAL> is larger than
the C<START>.

=item C<$self-E<gt>label>

As I<getter>, this method returns the edge's label. Legal results may
include C<undef>.

=item C<$self-E<gt>label( $new_label );>

As I<setter>, this method sets a new label on an edge.

=item C<$self-E<gt>start;>

As I<getter>, this method returns the edge's start point. You must not
change the start point except through construction.

=item C<$self-E<gt>final;>

As I<getter>, this method returns the edge's end point. You must not
change the end point except through construction.

=item C<$self-E<gt>show;>

=item C<$self-E<gt>show( KEY => $value, ... );>

This method returns a string representing the edge. Label and feature
values are C-escaped strings.

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
