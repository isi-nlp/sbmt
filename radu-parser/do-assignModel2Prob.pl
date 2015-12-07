#!/usr/usc/bin/perl
 
use strict;
use Getopt::Long;

### arguments ####################################################
&usage() if (($#ARGV+1) < 4); # die if too few args

my $infile;                     # name of input file
my $outfile;                    # output file
my $vocfile;                    # output file
my $cnt;                        # sentence count

GetOptions("help" => \&usage,
        "in=s" => \$infile, # Input file
           "out=s" => \$outfile, # output file
           "voc=s" => \$vocfile, # vocabulary file
           "cnt=s" => \$cnt, # sentence count
           );
                                                                                                               
sub usage() {
  print "Usage: do-assignModel2Prob.pl ...\n\n";
  print "\t--help\t\tGet this help\n";
  print "\t--in <file>\t\tRead tree <file>\n";
  print "\t--out <file>\t\tWrite output to <file>\n";
  print "\t--voc <file>\t\tVocabulary <file>\n";
  print "\t--cnt <number>\t\tSpecify which foreign sentence is translated\n";                                                                                                               
  exit(0);
}
                                                                                                   
open(F, "$infile") or die "Cannot open $infile\n";

my $jname = "$infile.JNTN";
system("cp $infile $jname; echo '==$cnt==' >> $jname");
system("/auto/hpc-22/dmarcu/nlg/summer04/resources/radu-parser/radu-parser-v4.0/assignModel2Prob $jname 2 PTB2-MT 6 1 $vocfile $cnt > $outfile")==0 or die "Error in assignModel2Prob\n";
system("rm $jname");
