#!/usr/bin/env perl

#
# Author: graehl
# Created: Tue Aug 24 16:42:04 PDT 2004
#
# see usage (@end)
#

#use strict;
#use warnings;

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

my $fieldname='count';

my $infile_name; # name of input file
my $threshold=0;
my $percentile=
my $count;
GetOptions("help" => \&usage,
           "fieldname=s" => \$fieldname,
           "threshold:s" => \$threshold,
           "percentile=f" => \$percentile,
           "count" => \$count,
); # Note: GetOptions also allows short options (eg: -h)


sub usage() {
    pod2usage(-exitstatus => 0, -verbose => 2);
#  print "Usage: getlines_threshold.pl [options] index1 index2 index3... \n\n";
#  print "\t--help\t\tGet this help\n";
#  print "\t--idregexp\t\t(default is "id=(\d+)")\n";
#  print "Selects (in sorted order) lines with id=#index1, id=#index2,... from STDIN";
  exit(1);
}


### main program ################################################

sub ln_weight {
    my ($x)=@_;
    no warnings 'numeric';
    if  ($x =~ /e\^(.*)$/) {
        return $1;
    } elsif ($x > 0) {
        return log $x;
    } else {
        return -1e+999999;
    }
}

my $ln_thresh=&ln_weight($threshold);
my $c=0;
my $N=0;
my $sum=0;
my $sumsq=0;
my $maxln=0;
my $max=0;

while(<>) {
    if (/$fieldname=(\S*)/) {
        ++$N;      
        my $val=$1;
        my $lnval=&ln_weight($val);
        $sum+=exp($lnval);
        $sumsq+=exp($lnval+$lnval);
        if ($maxln < $lnval) {
            $maxln=$lnval; $max=$val;
        }
        if ($lnval > $ln_thresh) {
            ++$c; 
            print if (!$count);
        }            
    } else {
        print if (!$count);
    }
}

sub variance {
    my ($sum,$sumsq,$n)=@_;
    my $v=($sumsq-$sum*$sum/$n)/$n; # 1/n(sumsq)-(mean)^2
    return ($v>0?$v:0);
}

my $out = "No lines containing $fieldname=...\n";
if ($N > 0) {
    my $avg=$sum/$N;
    my $var=&variance($sum,$sumsq,$N);
    my $stddev=sqrt($var);
    my $stderr=$stddev/sqrt($N);
    
    $out="For attribute $fieldname:\nmax=$max\nmean=$avg\nvariance=$var\nstddev=$stddev\nstderr=$stderr\n$c out of $N items exceeded threshold $threshold.\n";
}
print STDERR $out;
if ($count) {
    print "$c\n",$out;
}
__END__

=head1 NAME

    getlines_threshold.pl

=head1 SYNOPSIS

    getlines_threshold.pl [options] threshold

     Options:
       --help          
       --fieldname name
       --count
       --threshold W
    
=head1 OPTIONS

=over 8

=item B<-help>

    Print a brief help message and exits.

=item B<--threshold W>
    Sets value that must be exceeded for fieldname
    
=item B<--fieldname name>

    Looks for /name=(\s+)/ on input, then compares the captured value $1 against
    threshold; if $1 is greater, the line is selected.  Note that name
    is interpolated into a Perl regexp.

=item B<--count>

    Instead of copying input lines, count them and print the final count

=item B<--percentile percent> E.g. 95 would estimate a threshold that could be
    used in another run, to select about 95% of the items.

    
=back

=head1 DESCRIPTION

    Selects or counts lines whose "fieldname" attribute value is greater than or
    equal to threshold.  Values are either nonngative real numbers, or e^(real
    number) (e.g. 0.1, e^-1.2e+30).  If the fieldname attribute doesn't exist,
    the line is passed through (unless, of course, you specify --count)

=cut
