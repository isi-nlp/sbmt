#
# Provides some common useful tools frequently required.
#
# Author: Jens-S. Vöckler voeckler at isi dot edu
# $Id$
#
package NLPTools;
use 5.006;
use strict;

require Exporter;
our @ISA = qw(Exporter);

# declarations of methods here. Use the commented body to unconfuse emacs
sub commas($);			# { }
sub debug;			# { }
sub footprint(;@);		# { }
sub pageinfo(;@);		# { }
sub find_exec($;@); 		# { }

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
# The tags allows declaration       use NLPTools ':all';
our @EXPORT_OK = qw(commas debug footprint pageinfo find_exec);
our %EXPORT_TAGS = ( 'all' => [ @EXPORT_OK ] );
our @EXPORT = qw();

our $VERSION = '1.0';
$VERSION = $1 if ( '$Revision$' =~ /Revision:\s+([0-9.]+)/ );

# Preloaded methods go here.
use File::Spec;

sub read_vmstat {
    # purpose: Read the /proc/vmstat file
    # returns: hash with "pg[a-z]+" keys and numerical values
    #
    local(*PROC);
    my %result = ();
    if ( lc($^O) eq 'linux' && open( PROC, "</proc/vmstat" ) ) {
	while ( <PROC> ) {
	    $result{$1} = $2 if /^(p[gs][a-z]+)\s(\d+)/;
	}
	close PROC;
    }
    wantarray ? %result : $result{pswpin}+$result{pswpout};
}

sub diff_vmstat(;\%) {
    # purpose: Diff current state with start state
    # paramtr: %h (opt. IN): hash with current state. If no arg, obtain current.
    # globals: %NLPTools::vmstat (IN): start state
    # returns: diff as hash
    #
    my $xref = shift || { read_vmstat() };
    my %result = ();
    foreach my $k ( %{$xref} ) {
	$result{$k} = $xref->{$k} - $NLPTools::vmstat{$k};
    }
    %result;
}

BEGIN {
    # Warning: Heavy magic! Attempt to use Time::HiRes qw(time),
    # but do not fail, if it is not available
    eval "require Time::HiRes;";
    unless ( $@ ) {
	if ( Time::HiRes->can('import') ) {
	    $__PACKAGE__::hires = 1;
	    Time::HiRes->import( 'time' );
	}
    }

    # remember swapping and paging counters at program start
    %NLPTools::vmstat = read_vmstat() if ( lc($^O) eq 'linux' );
}


sub commas($) {
    my $text = reverse shift();
    $text =~ s/(\d{3})(?=\d)(?!\d*\.)/$1,/g;
    return scalar reverse $text;
}

sub debug {
    # purpose: print an arbitrary informational message
    my $now = time();
    my @now = localtime( int($now) );
    my $msg = sprintf( "# %04d-%02d-%02d %02d:%02d:%02d",
		       $now[5]+1900, $now[4]+1, @now[3,2,1,0] );
    $msg .= substr(sprintf("%.3f",($now-int($now))),1) if $__PACKAGE__::hires;
    print STDERR $msg, ' ', join('',@_), "\n";
}

my %mapping = ( io => 'io',  pgmajfault => 'flt',
		pswpin => 'si', pswpout => 'so', 
		pgpgin => 'pi', pgpgout => 'po' );

sub pageinfo(;@) {
    # purpose: log paging and swapping activity of system -- Linux only 
    # paramtr: (opt. IN): any number of scalars to use as message prefix
    # warning: assumes 4kB page size
    # globals: %NLPTools::vmstat (IN): original system activity state
    # returns: the activity hash, or just the disk-I/O incurring operations
    #
    my %x = ();
    if ( lc($^O) eq 'linux' ) {
	%x = diff_vmstat ;
	$x{io} = $x{pswpin}+$x{pswpout}+$x{pgmajfault};

	my $msg = '';
	foreach my $k ( sort keys %mapping ) {
	    $msg .= ', ' if length($msg);
	    $msg .= ( exists $mapping{$k} ? $mapping{$k} : $k );
	    $msg .= "=$x{$k}";
	}
	my $prefix = @_ ? join('',@_) : 'pageinfo';
	debug "$prefix [#]: $msg";
    }
    wantarray ? %x : $x{io};
}

sub footprint(;@) {
    # purpose: log memory usage for self -- Linux only 
    # paramtr: (opt. IN): any number of scalars to use as message prefix
    # warning: assumes 4kB page size
    # returns: size ->   process vmsize
    #          rss ->    process rss
    #          share ->  process shared
    #          exe ->    executable
    #          lrs ->    local?
    #          data ->   data
    #          dirty ->  dirty pages (writable)
    #
    my %x = ();
    if ( lc($^O) eq 'linux' ) {
	local(*PROC);
	if ( open( PROC, "</proc/$$/statm" ) ) {
	    my $line;
	    if ( defined ($line = <PROC>) ) {
		chomp($line);
		# FIXME: assumes pages of 2^12 (1 M = 2^20)
		@x{'size','rss','share','exe','lrs','data','dirty'} = 
		    map { sprintf( "%.1f", $_ / 256.0 ) } split /\s+/,$line ; 

		my $msg = '';
		foreach my $k ( qw(size rss share data) ) {
		    $msg .= ', ' if length($msg);
		    $msg .= "$k=$x{$k}";
		}
		my $prefix = @_ ? join('',@_) : 'meminfo';
		debug "$prefix [MB]: $msg";
	    }
	    close PROC;
	}
    }
    %x;
}

sub find_exec($;@) {
    # purpose: determine location of given binary in $PATH
    # paramtr: $program (IN): executable basename to look for
    #          @other (opt. IN): also check these directories FIRST!
    # returns: fully qualified path to binary, undef if not found
    #
    my $program = shift;
    my @path = File::Spec->path;
    unshift( @path, @_ ) if @_ > 0;
    foreach my $dir ( @path ) {
        my $fs = File::Spec->catfile( $dir, $program );
        return $fs if -x $fs;
    }
    undef;
}

1;

__END__

=head1 NAME

NLPTools - Some useful tools when writing perl scripts

=head1 SYNOPSIS

    use File::Basename;
    use lib dirname($0);
    use NLPTools;

    $x = 12345678910.1112;
    $s = commas($x);            # 123,456,678,910.1112

    debug $s;                   # create msg on STDERR
    footprint;                  # meminfo, uses debug
    pageinfo;                   # pg/swp info, uses debug

    $date = find_exec('date');
    $booh = find_exec('booh',"$ENV{HOME}/bin");

=head1 DESCRIPTION

This module provides some generically useful tools when writing 
larger scripts. Unfortunately, it also provides a very specific 
function targeted as rule processing. 

=head1 FUNCTIONS

=over 4

=item debug( .. )

The C<debug> function prints a message, comprised of any number of
arguments submitted to C<debug>. The message is prefixed with a hash
(C<#>) sign and the current time stamp this log message was generated,
possibly with a millisecond resolution, if C<Time::HiRes> is available.

If the C<Time::HiRes> module is availabe, a typical log message may look
like this:

    # 2007-07-26 11:11:24.228 counting source words

If C<Time::HiRes> could not be loaded, the same instruction would have
generated the following log message:


    # 2007-07-26 11:11:24 counting source words

All log messages go to C<STDERR>. 

=item [%m =] footprint;

=item [%m =] footprint( $msg );

=item [%m =] footprint( $a, $b, .. );

The C<footprint> function only works on Linux systems. B<Warning:> It
assumes that the page size is 4kB. The API may break with any kernel
other than 2.6. 

C<footprint> employs the C<debug> function above to print its output. If
called without any arguments, it uses a default message string
I<meminfo>, otherwise the concatination of its arguments.

After the message, a string C<[MB]:> is appended, followed by the memory
consumption of the current process in MB. The four values are the total
size, the resident set size, the size of shared and sharable pages, and
the size of all data.

The return value of the C<footprint> function, if desired, is a hash of
all known memory sizes to their current values. The values are taken
from the F</proc/$$/statm> file, and include I<size>, I<rss>, I<share>,
I<exe>, I<lrs>, I<data>, and I<dirty>. 

There is currently no mode where only the data is returned without
printing. The API may change in the future, or be divided.



=item [%v =] pageinfo;

=item [%v =] pageinfo( $msg );

=item [%v =] pageinfo( $a, $b, .. );


The C<pageinfo> function only works on Linux systems. B<Warning:> The
API may break with any kernel release other than v2.6.

At load-time of the C<NLPTools> module, a snap-shot of the current system
activity counters is taken as the baseline. Any time the C<pageinfo>
function is called, the snap-shot is subtracted from the current
activity state counters. There is (yet) no API to update the snap-shot
state.

C<pageinfo> employs the C<debug> function above to print its output. If
called without any arguments, it uses a default message string
I<pageinfo>, otherwise the concatination of its arguments.

After the message, a string C<pageinfo [#]:> is appended, followed by
the virtual memory I<system> activity in pages. The rest of the message
contains the page difference as follows

=over 4

=item io

designates the sum of known disk-I/O activity. It is the sum of the
swap-in, swap-out and major page faults.

=item flt

are the major page faults, also known as true page faults. 

=item si

designates swap-in operations. 

=item so

designates swap-out operations. 

=item pi 

designates page-in operations. Page-activity is normal when doing
file-I/O.

=item po

designates page-out operations. Page-activity is normal when doing
file-I/O.

=back 4

The return value of the C<pageinfo> function, if desired, is a hash of
all known system activity pertaining to virtual memory. The values are
taken from the F</proc/vmstat> file, and include any full word without
underscores that start with prefix C<pg> or C<ps>. 

There is currently no mode where only the data is returned without
printing. The API may change in the future, or be divided.


=item $exe = find_exec( $basename );

=item $exe = find_exec( $basename, @dir );

The C<find_exec> function searches the C<PATH> environment variable for
the existence of an executable file with the given base name. The first
argument must not contain any paths, just the name of the application.

If additional paths outside the C<PATH> variable are to be considered,
any number of paths may be added as further arguments.

The function tries each element, first of the C<PATH>, and then any
optional extra directories, as prefix for the basename. It exits with
the first executable file that is thus found. If no match is found, the
function will return the C<undef> value.

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

Copyright 2007 The University of Southern California. All rights
reserved.

=cut
