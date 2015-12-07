#
# base class to "mark" everything lattice
#
package Lattice::Base;
use v5.8.8;
use strict;
use vars qw($AUTOLOAD);

require Exporter;
our @ISA = qw(Exporter);

our @EXPORT_OK = qw();
our %EXPORT_TAGS = ( 'all' => [ @EXPORT_OK ] );
our @EXPORT = qw();

our $VERSION = '1.0';
$VERSION = $1 if ( '$Revision$' =~ /Revision:\s+([0-9.]+)/ );

use Carp;

sub new {
    my $proto = shift;
    my $class = $proto || ref $proto || __PACKAGE__;
    bless { @_ }, $class;
}

sub show {
    my $self = shift;
    confess( ref($self), " did not overwrite its show() method" );
}

sub feature {
    my $self = shift;
    my $key = shift || confess "feature name is a required argument";

    # avoid auto-vivification!
    my $result = exists $self->{FEATS} && exists $self->{FEATS}->{$key} ? 
	$self->{FEATS}->{$key} : 
	undef;

    $self->{FEATS}->{$key} = shift if @_;
    $result;
}

sub features {
    my $self = shift;
    my $result = $self->{FEATS};

    if ( @_ ) {
	if ( ref $_[0] eq 'HASH' ) {
	    # user passed hash reference as input
	    $self->{FEATS} = shift;
	    $result;
	} elsif ( ( scalar(@_) & 1 ) == 0 ) {
	    # user passed explicit hash as input
	    $self->{FEATS} = { @_ };
	    defined $result ? %{$result} : {}
	} else {
	    confess( ref($self), " called method features illegally" );
	}
    }
}

sub AUTOLOAD {
    my $self = shift;
    my $type = ref($self) || 
	confess "\"$self\" is not an object (\$AUTOLOAD=$AUTOLOAD)";

    my $name = uc($AUTOLOAD);
    $name =~ s/.*:://;   # strip fully-qualified portion

    # avoid auto-vivification
    my $result = exists $self->{$name} ? $self->{$name} : undef;
    $self->{$name} = shift if (@_);
    $result;    
}

1;

__END__

=head1 NAME

Lattice::Base - base class for functionality shared by lattice constituents.

=head1 SYNOPSIS

This class it not meant to be instantiated directly. However, it provides
a convenient source to check, if a given object C<isa> lattice constituent. 

=head1 DESCRIPTION

This class creates a number of fundamental methods shared by all
constituents. 

=head2 METHODS

=over 4

=item C<$self-E<gt>show>

This method is abstract. If ever called in the base class, and not
overwritten by the constituent, it will abort execution.

=item C<$self-E<gt>feature( $key );>

As a I<getter>, this method determines the current setting for the given
key C<$key>. Care is taken to avoid auto-vivification of values. The
(legal) value C<undef> for an existing key C<$key> is indistinguishable
from the non-existing key C<$key>. 

=item C<$self-E<gt>feature( $key, $value );>

As a I<setter>, this method creates a new key, or replaces an existing
key, with a new value. The previous value is the method's return value.
The (legal) value C<undef> for an existing key C<$key> is
indistinguishable from the non-existing key C<$key>.

=item C<$self-E<gt>features;>

As a I<getter>, this method returns a reference to the current hash of
features. This hash reference may be empty or even C<undef>. 

=item C<$self-E<gt>features( $href );>

=item C<$self-E<gt>features( %hash );>

=item C<$self-E<gt>features( $k1, $v1, $k2, $v2, ... );>

As a I<setter>, this method replaces the existing hash of features with
the new features. The new features may either be specified as single
hash reference argument, or as list-expanded hash (read: You can use a
hash, or an even-numbered list of key-value pairs).

The return value is the previous setting, which may be a reference to an
empty hash, or even C<undef>.

=item C<AUTOLOAD>

The C<AUTOLOAD> method provides the magic to all constituents, to that
it will not be necessary to code getters and setters for member
(instance) variables. This assumes that all child classes use the hash
reference approach to record members. Any sibling is free to provide a
concrete method.

=back 

=head1 SEE ALSO

L<Lattice::Node>, L<Lattice::Edge>, L<Lattice::Block>,
L<Lattice::Graph>.

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
