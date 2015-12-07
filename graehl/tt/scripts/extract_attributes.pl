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

#use WebDebug qw(:all);

### arguments ####################################################
&usage() if (($#ARGV+1) < 1); # die if too few args

my $fieldname='';


GetOptions("help" => \&usage,
           "fieldname=s" => \$fieldname,
); # Note: GetOptions also allows short options (eg: -h)


sub usage() {
    pod2usage(-exitstatus => 0, -verbose => 2);
#  print "Usage: extract_attributes.pl [options] index1 index2 index3... \n\n";
#  print "\t--help\t\tGet this help\n";
#  print "\t--idregexp\t\t(default is "id=(\d+)")\n";
#  print "Selects (in sorted order) lines with id=#index1, id=#index2,... from STDIN";
  exit(1);
}


### main program ################################################

while(<>) {
    if (/\Q$fieldname\E=(\S*)/o) {
        print $1,"\n";
    }
}

__END__

=head1 NAME

    extract_attributes.pl

=head1 SYNOPSIS

    extract_attributes.pl [options] index1 index2 index3...

     Options:
       --help          
       --fieldname field

=head1 OPTIONS

=over 8

=item B<-help>

    Print a brief help message and exits.

=item B<--fieldname field>

    Looks for field=value

=back

=head1 DESCRIPTION

    Outputs only the value for a given field (one per line).
    
=cut  
