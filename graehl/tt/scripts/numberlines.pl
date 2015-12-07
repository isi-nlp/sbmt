#!/bin/sh
#! -*- perl -*-
 eval 'exec perl -w -x $0 ${1+"$@"} ;'
  if 0;
#
# Author: graehl
# Created: Tue Aug 24 16:42:58 PDT 2004
#
# This script ...
#

use strict;
use Getopt::Long;
use File::Basename;

### script info ##################################################
my $scriptdir; # location of script
my $scriptname; # filename of script

BEGIN {
   $scriptdir = &File::Basename::dirname($0);
   ($scriptname) = &File::Basename::fileparse($0);
   push @INC, $scriptdir; # now you can say: require "blah.pl" from the current directory
}

use WebDebug qw(:all);

sub usage {
  print "simply appends a #line number to every line that starts with a number, starting from 1.\n";
  exit(1);
}
GetOptions("help" => \&usage,
	); # Note: GetOptions also allows short options (eg: -h)



### main program ################################################
my $numberno=1;
while(<>) {
  if (/^\s*[\-0-9]/) {
    chop;
    print $_," #",$numberno++,"\n"; 
  }
}
