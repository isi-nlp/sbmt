#!/usr/bin/perl -w
#
# Author: ithayer
# Created: Mon Jun 14 11:09:11 PDT 2004
#
# This script runs radu's parser. The --in flag specifies the input, prb-format [Nbest] tree file. the --out specifeis the output model-prob assigned file. 
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

my $DEBUG=$ENV{DEBUG}; # control debugging output with the DEBUG environment variable

sub debug {
   print STDERR join(',',@_),"\n" if $DEBUG;
}

### arguments ####################################################
&usage() if (($#ARGV+1) < 3); # die if too few args

my $infile;			# name of input file
my $outfile;			# output file
my $parserdir;			# parser directory
my $rtype;			# resource type

GetOptions("help" => \&usage,
	"in=s" => \$infile, # Input file
	   "out=s" => \$outfile, # output file
	   "parserdir=s" => \$parserdir, # parser dir
	   "rtype=s" => \$rtype # resource type
	   );

sub usage() {
  print "Usage: radu-parse.pl ...\n\n";
  print "\t--help\t\tGet this help\n";
  print "\t--in <file>\t\tRead ospl, mt tokenized <file>\n";
  print "\t--out <file>\t\tWrite output to <file>\n";
  print "\t--parserdir <dir>\t\tLook for radu's parser in <dir>\n";
  print "\t--rtype <dir>\t\tSpecify PTB|MT|PTB-MT\n";

  exit(0);
}


### main program ################################################

die "Please specify an infile and an outfile" if !defined($infile) or !defined($outfile);
die "Please specify the directory where the parser is located" if !defined($parserdir);
die "Please specify the type of resource & output to be used: PTB|MT|PTB-MT" if !defined($rtype);

print STDERR "Reading from $infile, writing to $outfile\n";

my $JAVA = "../../jre117_v3/bin/jre -cp";
my $tmpname = `mktemp /tmp/RADUPARSE.XXXXXX`;
chomp($tmpname);

print STDERR "Tempfile: $tmpname\n";

open(IN, $infile) or die "Couldn't open $infile";

my $max_lines = 10000;
my $line_counter=0;

system("rm -f $outfile");

open(TMP, ">$tmpname.PRB");

while (my $line = <IN>) { 
  print TMP $line;

  if( $line =~ /^==/ ){
      $line_counter++;
  }
  if ($line_counter==$max_lines) { 
    close(TMP);
    do_assign();
    open(TMP, ">$tmpname.PRB");
    $line_counter=0;
  }
}
close(TMP);
if ($line_counter>0) { 
    do_assign();
}

sub do_assign()
{
    my $VAR = 0;
    if( $VAR ){
	for(my $i=1; $i<=50; $i+=5){
	    system("cd $parserdir; ./assignModel2Prob $tmpname.PRB 1 $rtype $i $VAR");
	}
    }
    else{
	system("cd $parserdir; ./assignModel2Prob $tmpname.PRB 1 $rtype 46 $VAR >> $outfile ");
    }
}
