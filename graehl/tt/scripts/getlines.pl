#!/usr/bin/env perl

#
# Author: graehl
# Created: Tue Aug 24 16:42:04 PDT 2004
#
# This script ...
#

use strict;
use warnings;
use Getopt::Long;
use File::Basename;

my $DEBUG=$ENV{DEBUG};
sub debug {
    print STDERR join(',',@_),"\n" if $DEBUG;
}

### script info ##################################################
my $scriptdir; # location of script
my $scriptname; # filename of script

BEGIN {
   $scriptdir = &File::Basename::dirname($0);
   ($scriptname) = &File::Basename::fileparse($0);
   push @INC, $scriptdir; # now you can say: require "blah.pl" from the current directory
}


### arguments ####################################################
&usage() if (($#ARGV+1) < 1); # die if too few args

my $infile_name; # name of input file
my $linesfile;
my $sort=1;
my $except;
my $drop_others;
my $matching='';
GetOptions("help" => \&usage,
           "linesfile=s" => \$linesfile,
           "except!" => \$except,
           "sortids!" => \$sort,
           "matching=s" => \$matching,
           "drop-not-matching!" => \$drop_others,
); # Note: GetOptions also allows short options (eg: -h)


sub usage() {
  print "Usage: getlines.pl [options] index1 index2 index3... \n\n";
  print "\t--linesfile FILE\tload line indices from FILE (instead of from command line)\n";
  print "\t--nosort  \tassume line indices are already sorted\n";
  print "\t--except  \tprint all *except* the indicated lines\n";
  print "\t--matching REGEXP \tconsider only lines matching perl REGEXP\n";
  print "\t--drop-not-matching\tdon't print lines that don't match regexp\n";
  print "Selects (in sorted order) line #index1, #index2 (index=1 for the first line) from STDIN; skipping an initial '(' line if any";
  exit(1);
}


### main program ################################################

my @indices=();
if ($linesfile) {
    open(LINES,'<',$linesfile) || die "no $linesfile - $!";
    if ($sort) {
        while (<LINES>) {
            push @indices,$1 while (/(\d+)/g);
        }
    }
} else {
    while (@ARGV && $ARGV[0] =~ /^\d+$/) {
        unshift @indices,(shift @ARGV);
    }
    &usage unless (scalar @indices);
}
@indices = sort { $a <=> $b }@indices if $sort;


my $lineline=undef;
my $indexi=0;
sub nextlineno {
    if ($linesfile && !$sort) {
        $lineline=<LINES> unless defined $lineline;
        return unless defined $lineline;
        if ($lineline =~ /(\d+)/g) {
            return $1;
        } else {
            $lineline=undef;
            return &nextlineno;
        }
    } else {
        return ($indexi < scalar @indices) ? $indices[$indexi++] : undef;
    }
}

my $i=0;
my $expecting=&nextlineno;
die "please give some line numbers ..." unless defined $expecting;
&debug("expecting $expecting");
my $reg=qr/$matching/;
while(<>) {
    if (/$reg/o) {
  ++$i;
  if ($i >= $expecting) {
      die "impossible: expected line $expecting and we're already on line $i without seeing it" unless $i == $expecting;
      &debug("expecting $expecting");
    print unless $except;
      $expecting=&nextlineno;
      if (!defined $expecting) {
          if ($except) {
              print while(<>);
          }
          exit;          
      }
      die "out of order lineno: expecting $expecting while on line $i" unless $expecting >= $i;
  } else {
      print if $except;
  }
} else {
    print unless $drop_others;
}
}
