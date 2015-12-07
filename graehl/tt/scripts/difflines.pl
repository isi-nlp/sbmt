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
my $BLOBS;

BEGIN {
   $scriptdir = &File::Basename::dirname($0);
   ($scriptname) = &File::Basename::fileparse($0);
   $ENV{BLOBS}='/home/hpc-22/dmarcu/nlg/blobs' unless exists $ENV{BLOBS};
   $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
   $BLOBS=$ENV{BLOBS};
   my $libgraehl="$BLOBS/libgraehl/unstable";
   unshift @INC,$libgraehl if -d $libgraehl;
   unshift @INC, $scriptdir;
}

require "libgraehl.pl";

#use WebDebug qw(:all);

### arguments ####################################################

my ($out1,$out)=('','-');
my $linenumbers;
my $maxdiffs;
my $eval;
my $compare;

GetOptions("help" => \&usage,
           "firstout:s" => \$out1,
           "out:s" => \$out,
           "linenumbers" => \$linenumbers,
           "maxdiffs:i" => \$maxdiffs,
           "evaltransform:s" => \$eval,
           "compare:s" => \$compare,
); # Note: GetOptions also allows short options (eg: -h)

sub usage() {
    print STDERR "Error: @_\n" if defined $_[0];
    pod2usage(-exitstatus => 0, -verbose => 2);
  exit(1);
}

### main program ################################################

&usage unless (scalar @ARGV == 2);
my %array=();
my ($in1,$in2)=(shift,shift);
my ($IN1,$IN2,$O1,$O2);
#&debug($IN1,$IN2);
$IN1=openz($in1);
$IN2=openz($in2);
$O2=outz_stdout($out);
$O1=$out1 ? openz_out($out1) : $O2;

my ($l1,$l2,$c1,$c2);

my $n=0;
my $m=0;
while (defined($l1=<$IN1>) && defined($l2=<$IN2>)) {
    ++$n;
    ($c1,$c2)=($l1,$l2);
    if (defined $eval) {
        $_=$c1;eval $eval;chomp;$c1=$_;
        $_=$c2;eval $eval;chomp;$c2=$_;
    }
    my $equal;
    if ($compare) {
        my ($a,$b)=($c1,$c2);
        $equal=eval $compare;
    } else {
        $equal=($c1 eq $c2);
    }
    if ($equal) {
        #&debug("equal",$c1,$c2,$n);
    } else {
        log_numbers("line #$n differs");
        ++$m;
        #&debug("different",$c1,$c2,$n);
        if ($linenumbers) {
            $l1 = "$n:$l1";
            $l2 = "$n:$l2";
        }
        print $O1 $l1;
        print $O2 $l2;
        if (defined($maxdiffs) && $m >= $maxdiffs) {
            print STDERR "Done: found the desired $m diffs after only $n lines.\n";
            exit;
        }
    }
}

if (defined($l1) || <$IN2>) {
    &warning((defined($l1) ? "First file $in1" : "Second file $in2")." had extra lines (the other had only $n)");
}

&all_summary;

print STDERR "Done: found $m diffs after $n lines.\n";


__END__

=head1 NAME

    difflines.pl

=head1 SYNOPSIS

    difflines.pl [options] input1 input2

     Options:
       --help
       --firstout filename (default '' -> same as out)
       --out filename (default STDOUT)
       --linenumbers
       --maxdiffs n
       --evaltransform 'perl code modifying $_' (e.g. 's/^\s+//' ignores leading whitespace)
       --compare 'perl code returning true if $a == $b' (e.g. '$a==$b') 
=head1 OPTIONS

=over 8

=item B<-help>

    Print a brief help message and exits.

    

=back

=head1 DESCRIPTION

     Assuming a 1 to 1 alignment of lines in files input1 and input2, writes to
     --firstout and --secondout respectively the lines that differed.

    Extra lines in one file or the other will result in a warning on STDERR;
    such lines won't appear in either of the difference files.

   If maxdiffs is specified, the remainder of the input won't be read once that
   many differences are found.

  If an -eval is supplied, the code should modify $_ (and the results will be
  used to check for equality - but the original lines will be written to the
  outputs)

=cut
