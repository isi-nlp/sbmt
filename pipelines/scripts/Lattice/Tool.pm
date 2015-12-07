#
# helper class for lattices
#
package Lattice::Tool;
use 5.008008;
use strict;
use warnings;

require Exporter;
our @ISA = qw(Exporter);

sub contents($);		# { } 
use constant START_SYMBOL => '<foreign-sentence>';

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead. Do
# not simply export all your public functions/methods/constants.

# This allows declaration       use Lattice::Tool ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.
our @EXPORT_OK = qw(contents START_SYMBOL);
our %EXPORT_TAGS = ( 'all' => [ @EXPORT_OK ] );
our @EXPORT = qw();

our $VERSION = '1.0';
$VERSION = $1 if ( '$Revision$' =~ /Revision:\s+([0-9.]+)/ );

use Carp;
use utf8;
use Lattice::Graph;
use Lattice::Parser;

sub contents($) {
    # purpose: read contents from file
    # paramtr: $fn (IN): filename
    # returns: file contents as single string
    #
    my $fn = shift;
    my $result;
    local(*IN);
    open( IN, "<$fn" ) || die "FATAL: open $fn: $!\n";
    binmode( IN, ':utf8' );	# ~@#!
    local $/ = undef;
    $result = <IN>;
    close IN;
    $result;
}

sub new { 
    my $proto = shift;
    my $class = $proto || ref $proto || __PACKAGE__;

    bless { parser => Lattice::Parser->new, @_ }, $class;
}

sub parser {
    my $self = shift;
    # avoid auto-vivification
    my $result = exists $self->{parser} ? $self->{parser} : undef;
    $self->{parser} = shift() if @_; # setter
    $result;
}

sub lattices_from_file {
    my $self = shift;
    my $fn = shift;
    $self->{parser}->lattices( contents($fn) );
}

sub lattice_from_file {
    my $self = shift;
    my $fn = shift;
    $self->{parser}->lattice( contents($fn) );
}

sub lattices {
    my $self = shift;
    if ( @_ == 1 && ref $_[0] eq 'SCALAR' ) {
	# passing ref to text
	my $sref = shift;
	$self->{parser}->lattices($sref);
    } else {
	# passing 1..N lines
	$self->{parser}->lattices( join('',@_) );
    }
}

sub lattice {
    my $self = shift;
    if ( @_ == 1 && ref $_[0] eq 'SCALAR' ) {
	# passing ref to text
	my $sref = shift;
	$self->{parser}->lattice($sref);
    } else {
	# passing 1..N lines
	$self->{parser}->lattice( join('',@_) );
    }
}

1;

__END__

=head1 NAME

Lattice::Tool - small value-added helper for parsing lattices.

=head1 SYNOPSIS

Here an example how to parse multiple lattices from a single file: 

    use File::Basename;
    use lib dirname($0);
    use Lattice::Tool qw(START_SYMBOL);

    my $fn = shift || die "need a filename"; 

    my $tool = Lattice::Tool->new ;
    my $aref = $tool->lattices_from_file($fn);
    die "FATAL: Unable to parse lattices in $fn\n" 
	unless defined $aref;

    binmode( STDOUT, ':utf8' );
    foreach my $dag ( @{$aref} ) {
	$dag->delete_edge_by_text( START_SYMBOL );
	print $dag->show();
    }

Here an example how to parse a single lattice from a single file. Please
note the singular (instead of plural) in the method's name. You may also
use the singular method to parse just the first lattice from a file with 
one or more lattices: 

    use File::Basename;
    use lib dirname($0);
    use Lattice::Tool qw(START_SYMBOL);

    my $fn = shift || die "need a filename"; 

    my $tool = Lattice::Tool->new ;
    my $dag = $tool->lattice_from_file($fn);
    die "FATAL: Unable to parse lattice in $fn\n" 
	unless defined $dag;

    binmode( STDOUT, ':utf8' );
    $dag->delete_edge_by_text( START_SYMBOL );
    print $dag->show();

=head1 DESCRIPTION

This module's object provides instance and class methods to
simplify access to the RD-parser for lattices. You are free
to by-pass the helpers. 

=head2 EXPORTS

=over 4

=item C<contents( $filename )>

This generally available function will use a given name of a file. It
will read the file fully in UTF-8 mode. The function returns a single
string with all the contents of the file, including any line separators.

The function will die upon errors while opening the file.

=item C<START_SYMBOL>

This constant is the frequently referenced C<mini_decoder> start symbol.
It's current value is the string S<E<lt>foreign-sentenceE<gt>>.

=back

=head2 METHODS

=over 4

=item C<Lattice::Tool-E<gt>new>

The constructor instantiates a lattice parser instance internally. Use
the C<parser> accessor to get or set values. Use the lattice parser
functions to obtain lattices without having to deal with the parser.

=item C<$self-E<gt>parser;>

=item C<$self-E<gt>parser( $new_parser );>

This accessor method returns the current parser always. If called as
setter, the new parser instance is the only argument. In setter mode,
the new parser better be a child of L<Parse::RecDescent>. 

=item C<$self-E<gt>lattices( @lines );>

This method parses one or more lattice description from one or more
input lines. All lines are internally concatinated to a single string
before parsing. Line terminators do not matter in the context-free
lattice grammar. If you pass a reference to a single string as only
argument, this string is modified as it is parsed.

The metod returns a reference to an array of L<Lattice::Graph>
instances. If the parsing failed, the C<undef> value is returned. 

=item C<$self-E<gt>lattice( @lines );>

This method parses a single lattice from one or more input lines. All
lines are internally concatinated to a single string before parsing.
Line terminators do not matter in the context-free lattice grammar. If
you pass a reference to a single string as only argument, this string is
modified as it is parsed.

The metod returns a L<Lattice::Graph> instance. If the parsing failed,
the C<undef> value is returned.

=item C<$self-E<gt>lattices_from_file( $filename );>

This method reads the contents of the given C<$filename> into memory,
using the L<contents> function above, and parses one or more lattices
from it, using the L<lattices> method. 

It returns a reference to an array of L<Lattice::Graph> instances. If
the parsing failed, the return value is C<undef>.

=item C<$self-E<gt>lattice_from_file( $filename );>

This method reads the contents of the given C<$filename> into memory,
using the L<contents> function above, and parses a single lattice
from it, using the L<lattice> method. 

It returns an L<Lattice::Graph> instances. If the parsing failed, the
return value is C<undef>.

=back

=head1 SEE ALSO

L<Lattice::Graph>, L<Lattice::Parser>, L<Parse::RecDescent>. 

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
