#!/usr/bin/env perl

#
# Author: graehl
# Created: Tue Aug 24 16:42:04 PDT 2004
#
# see usage (@end)
#

#use strict;
#use warnings;

use strict;

use Getopt::Long;
use Pod::Usage;
use File::Basename;

### script info ##################################################
my $scriptdir; # location of script
my $scriptname; # filename of script

BEGIN {
   $scriptdir = &File::Basename::dirname($0);
   ($scriptname) = &File::Basename::fileparse($0);
   push @INC, $scriptdir; # now you can say: require "blah.pl" from the current directory
}

require "exec_filter.pl";

#use WebDebug qw(:all);
my $DEBUG=$ENV{DEBUG};
sub debug {
    print STDERR join(',',@_),"\n" if $DEBUG;
}

use Encode;

### arguments ####################################################

my $begreg='^digraph';
my $pre='';

GetOptions("help" => \&usage,
           "outprefix=s" => \$pre,
           "beginregexp:s" => \$begreg,
); # Note: GetOptions also allows short options (eg: -h)


sub usage() {
    pod2usage(-exitstatus => 0, -verbose => 2);
  exit(1);
}


### main program ################################################

binmode STDIN, ":utf8";
my $n=0;
while(<>) {
#    &debug($_,$begreg,/$begreg/);
    if (/$begreg/o) {
        ++$n;
        &debug("$pre.$n");
        open P,">:encoding(utf8)","$pre.$n" or die;
    }
    print P if $n > 0;
}

__END__

=head1 NAME

    split_dot.pl

=head1 SYNOPSIS

   split_dot.pl -o outprefix [-b 'beginregexp']

=head1 OPTIONS

    -b - default 'digraph'
    
=over 8

=item B<-help>

    Print a brief help message and exits.

=back

=head1 DESCRIPTION

Each time beginregexp is seen, open a file outprefix.N (starting from N=1) and
    copy lines until next prefix is seen (or EOF) =cut
