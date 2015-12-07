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
use warnings;
use Getopt::Long;
use File::Basename;

my $blobbase="/home/hpc-22/dmarcu/nlg/blobs";

### script info ##################################################
my $scriptdir;                  # location of script
my $scriptname;                 # filename of script
my $BLOBS;

BEGIN {
    $scriptdir = &File::Basename::dirname($0);
    ($scriptname) = &File::Basename::fileparse($0);
    push @INC, $scriptdir;      # now you can say: require "blah.pl" from the current directory
    $ENV{BLOBS}='/home/hpc-22/dmarcu/nlg/blobs' unless exists $ENV{BLOBS};
    $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
    $BLOBS=$ENV{BLOBS};
    my $libgraehl="$BLOBS/libgraehl/unstable";
    push @INC,$libgraehl if -d $libgraehl;
}

require "libgraehl.pl";

### arguments ####################################################

use Pod::Usage;

### arguments ####################################################
&usage() if (($#ARGV+1) < 1); # die if too few args

my $period=1;

my $infile_name; # name of input file

GetOptions("help" => \&usage,
           "period:i" => \$period,
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

my $i=0;
my $last;
&argvz;
while(<>) {
    ++$i;
    if ($i % $period == 1) {
        print "$i $_";
        $last=undef;
    } else {
        $last=$_;
    }
}
print "$i $last" if (defined $last);

__END__

=head1 NAME

    order_gnuplot.pl

=head1 SYNOPSIS

    order_gnuplot.pl [options]      

=head1 OPTIONS

=over 8

=item B<-help>

    Print a brief help message and exits.

=item B<-period interval>

    Sample every <interval> lines
    

=back

=head1 DESCRIPTION

    Creates gnuplot data for cumulative order statistic graph (assumes one
    number per line on inputs, sorted) - prepends the line number (= order) to
    each line.

=cut
