#!/usr/bin/perl -w

# This script runs the POS tagger. The --in flag specifies the input, ospl, mt tokenized file. the --out specifeis the output tagged file. 

use strict;
use Getopt::Long;
use File::Basename;

### arguments ####################################################
&usage() if (($#ARGV+1) < 2); # die if too few args

my $infile;			# name of input file
my $outfile;			# output file
my $taggerdir;			# tagger directory

GetOptions("help" => \&usage,
	"in=s" => \$infile, # Input file
	   "out=s" => \$outfile, # output file
	   "taggerdir=s" => \$taggerdir, # tagger dir
	   );

sub usage() {
  print "Usage: radu-tag.pl ...\n\n";
  print "\t--help\t\tGet this help\n";
  print "\t--in <file>\t\tRead ospl, mt tokenized <file>\n";
  print "\t--out <file>\t\tWrite output to <file>\n";
  print "\t--taggerdir <dir>\t\tLook for tagger in <dir>\n";

  exit(0);
}


### main program ################################################

die "Please specify an infile and an outfile" if !defined($infile) or !defined($outfile);
die "Please specify the directory where the tagger is located" if !defined($taggerdir);

print STDERR "Reading from $infile, writing to $outfile\n";

my $JAVA = "../../jre117_v3/bin/jre -cp";
my $tmpname = `mktemp /tmp/RADUTAG.XXXXXX`;
chomp($tmpname);
print STDERR "Tempfile: $tmpname\n";

system("cat $infile > $tmpname.SNT");
system("cd $taggerdir; cat $tmpname.SNT | perl transformMTTok2PTBTok.pl | ./runMXPOST2.csh | perl transformRatn2Coll.pl &> $tmpname.POS");
system("cat $tmpname.POS > $outfile");
