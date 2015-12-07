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

require "libgraehl.pl";


### arguments ####################################################

my $id_regexp='id=(-?\d+)';

my $infile_name; # name of input file
my $fieldname;
my $arrayfile;
my $arrayregexp='';

GetOptions("help" => \&usage,
           "idregexp:s" => \$id_regexp,
           "arrayfile:s" => \$arrayfile,
           "keeparraylines:s" => \$arrayregexp,
           "fieldname:s" => \$fieldname,
); # Note: GetOptions also allows short options (eg: -h)

&usage() if ((scalar @ARGV) < 1); # die if too few args

sub usage() {
    print STDERR "Error: @_\n" if defined $_[0];
    pod2usage(-exitstatus => 0, -verbose => 2);
  exit(1);
}

### main program ################################################
my %indices=();
while (@ARGV && $ARGV[0] != 0) {
    my $id=shift @ARGV;
    $indices{$id}=0;
}
&usage unless (scalar %indices);
my %array=();
my $ARRAY;
if (defined $arrayfile) {
    &usage("fieldname required with arrayfile") unless defined $fieldname;
    &debug("arrayfile",$arrayfile);
    $ARRAY=openz($arrayfile);

    my @indices=sort { $a <=> $b } keys %indices;
    &debug("indices",@indices);
    my $i=0;
    while(<$ARRAY>) {
        next unless /$arrayregexp/o;
        ++$i;
        if ($i == $indices[0]) {
            chomp;
            $array{$i}=$_;
            shift @indices;
            last unless scalar @indices;
        }
    }
}

&argvz;
my $i=0;
while(<>) {
    if (/$id_regexp/o) {
        if (exists $indices{$1}) {
            ++$indices{$1};
            if (exists $array{$1}) {
                &debug("array",$1,$array{$1});
                my $val=$array{$1};
                chomp;
                print "$_ $fieldname=";
                $val =~ s/^\s+//;
                if ($val =~ /\s/) {
                    print "\"$val\"";
                } else {
                    print $val;
                }
                print "\n";
            } else {
                print;
            }
        }
    }
}

for (keys %indices) {
    my $n=$indices{$_};
    if ($n > 1) {
        warning("id $_ occured more than once.");
    } elsif ($n < 1) {
        warning("id $_ never occured.");
    }        
}

__END__

=head1 NAME

    getlines_id.pl

=head1 SYNOPSIS

    getlines_id.pl [options] index1 index2 index3... [non-integer-filename1] ...

     Options:
       --help          
       --idregexp regexp
       --arrayfile filename
       --keeparraylines regexp
       --field fieldname
    

=head1 OPTIONS

=over 8

=item B<-help>

    Print a brief help message and exits.

=item B<--idregexp 'regexp'>

    (default is "id=(\d+)").  $1 is the id checked against the user input
    

=back

=head1 DESCRIPTION

    Selects (in the order they appear) lines with id=#index1, id=#index2,... from STDIN.
    Optionally appends a fieldname=array[index] attribute (skipping lines that don't match keeparraylines)
    
=cut  
