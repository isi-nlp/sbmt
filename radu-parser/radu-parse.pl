#!/usr/bin/perl -w
#
# Author: ithayer
# Created: Mon Jun 14 11:09:11 PDT 2004
#
# This script runs radu's parser. The --in flag specifies the input, ospl, mt tokenized file. the --out specifeis the output parsed file. 
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

my $max_lines = 5000;
my $line_counter=0;
    
my $feedPOS = 0; # when feeding POS file

system("rm $outfile.PRB");
system("rm $outfile.VIEW");

if( !$feedPOS  ){
    system("rm $outfile.POS");
}

#system("rm $outfile.BEAM");
#system("rm $outfile.GBEAM");
#system("rm $outfile.TMD");
#system("rm $outfile.VIEW");

open(TMP, ">$tmpname.SNT");

while (my $line = <IN>) { 
  print TMP $line;

  $line_counter++;
  if ($line_counter==$max_lines) { 
    close(TMP);
    do_parse();
    open(TMP, ">$tmpname.SNT");
    $line_counter=0;
  }
  
}
close(TMP);
#die;
if ($line_counter>0) { 
    do_parse();
}

sub do_parse()
{
    if( $feedPOS ){
	;
    }
    elsif( $rtype eq "PTB" ){
	system("cd $parserdir; cat $tmpname.SNT | perl transformMTTok2PTBTok.pl | runMXPOST2.csh | perl transformRatn2Coll.pl &> $tmpname.POS");
    }
    else{
	system("cd $parserdir; cat $tmpname.SNT | perl transformMTTok2PTBTok.pl | runMXPOST2.csh | perl transformRatn2Coll.pl  | perl transformPTBPOS2MTPOS.pl &> $tmpname.POS");
    }
	
    if( $feedPOS ){
	system("cd $parserdir; ./parser $tmpname.SNT $rtype 1 0 20 0"); 
    }
    else{
	system("cat $tmpname.POS >> $outfile.POS");
	system("cd $parserdir; ./parser $tmpname.POS $rtype 1 0 20 0");
    }
    system("cat $tmpname.$rtype.PRB.0.20.0 >> $outfile.PRB");

    #system("cat $tmpname.$rtype.BEAM >> $outfile.BEAM");
    #system("cat $tmpname.$rtype.GBEAM >> $outfile.GBEAM");
    #system("cat $tmpname.$rtype.TMD.0.20.0 >> $outfile.TMD");
    system("cat $tmpname.$rtype.JNT.0.20.0 >> $outfile.VIEW");
}
