#!/usr/usc/bin/perl

use strict;
use Getopt::Long;
use File::Basename;

#my $command = "echo";
my $command = "~ithayer/bin/qsubrun.sh";

open(F, "$ARGV[0]") or die "Couldn't open $ARGV[0]";

my $fname = $ARGV[0];
$fname =~ /^(.*)\/(.*?)$/;
$fname = $2;

my $decoderhome = "/home/rcf-12/ithayer/projects/decoder-for-others/";
my $decoderversion = "12.16.04";

my $myhome = "/auto/hpc-22/dmarcu/nlg/summer04/resources/radu-parser/radu-parser-v3.2/";

my $tmpname = `mktemp $myhome/TEST/DCD1BY1.XXXXXX`;
chomp($tmpname);

print STDERR "Tempfile: $tmpname\n";

my $cnt = 1;
while( my $line=<F> ){
    open(O, ">$tmpname.$cnt") or die "Couldn't open $tmpname.$cnt";
    print O $line;
    close(O);
    system("cd $decoderhome$decoderversion; $command \"decoder.normal --params steve.params.tight --mode sblm --sblm-full-print yes --srt-pool-size 8000000 --edge-pool-size 10000000 --foreign $tmpname.$cnt --brf-file /auto/hpc-22/dmarcu/nlg/sdeneefe/data/rule-pruning/big3/big3.em.root.and.lhs.cp.lif.fixed.little0-16.no-lex5.pc-top128-km16.all/brf.stupid.rules.all-root_emprob.gz 2>&1 | gzip > $myhome/TEST/$fname.$decoderversion.$cnt.gz\"");
    $cnt++;
    #if( $cnt==3 ){ last; }
}
print STDERR "Done.\n";
