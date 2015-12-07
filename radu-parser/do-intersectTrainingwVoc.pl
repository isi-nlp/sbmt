#!/usr/usc/bin/perl

use strict;
use Getopt::Long;

my $command = "~sdeneefe/bin/rules/qsubrun-depend.sh";
my $sourcedir = "/auto/hpc-22/dmarcu/nlg/summer04/resources/radu-parser/radu-parser-v4.0/";

### arguments ####################################################
&usage() if (($#ARGV+1) < 2*5); # die if too few args

my $voc;                   
my $cnt; 
my $rtype;                      
my $thcnt;
my $dest;
my $dependency;

GetOptions("help" => \&usage,
           "voc=s" => \$voc, # prefix vocabulary file
           "cnt=s" => \$cnt,     # number of sentences
           "rtype=s" => \$rtype, # resource type
	   "thcnt=s" => \$thcnt, # unknown word threshold count
	   "dest=s" => \$dest ,  # destination directory
	   "dependency=s" => \$dependency ,  # dependency list for qsubrun
           );
                                                                                                               
sub usage() {
  print "Usage: do-assignModel2Prob.pl ...\n\n";
  print "\t--help\t\tGet this help\n";
  print "\t--voc <file>\t\tPrefix vocabulary <file>\n";
  print "\t--cnt <number>\t\tNumber of sentences\n";
  print "\t--rtype <PTB|PTB2-MT>\t\tResource type\n";
  print "\t--thcnt <number>\t\tUnknown word threshold count\n";                                      
  print "\t--dest <dir>\t\tDestination <dir>\n";                                                               print "\t[--dependency <list>]\t\tDependency <list>\n";                                                                                                               
  exit(0);
}
  
for(my $i=1; $i<=$cnt; $i++){
    print "intersectTrainingwVoc.pl $voc.$i $rtype $thcnt $dest $dependency\n";
    system("$command \"$sourcedir/intersectTrainingwVoc.pl $voc.$i $rtype $thcnt $dest\" $dependency");
    #if( $i>4 ){ last; }
}
