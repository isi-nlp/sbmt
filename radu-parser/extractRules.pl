#!/usr/usc/bin/perl

# initial version "ver0-chinese"
#    $SOURCE  = "/auto/hpc-22/dmarcu/nlg/summer04/resources/english-parse-trees/more.with-sentences.7.28.2004/trees";
#    $STEP = 200000;
#    $NLINES = 5962435;

# ver2.1-chinese source: /auto/hpc-22/dmarcu/nlg/blobs/ver2.1-chinese/training-data/large-chinese-eval04-v2.1.e-parse

$SOURCE = $ARGV[0];
$DESTDIR = $ARGV[1];

$NLINES = system("rm radu-tmp.cnt; wc -l $SOURCE > radu-tmp.cnt");
open(F, "radu-tmp.cnt") or mydie("Cannot open radu-tmp.cnt");
$line = <F>; 
if( $line =~ /^(.*?) (.*)$/ ){
    $NLINES = $1;
    print "Lines: $NLINES\n";
}
else{ mydie("Cannot extract \# lines from $line"); }

$STEP = 50000;
$psi = 0;
for($i=0; $i<$NLINES; $i=$ni){
    $ni = $i + $STEP;
    $si = $ni; 
    $si =~ s/000000$/M/;
    $si =~ s/000$/k/;
    print "trees2rules trees $i $ni > $DESTDIR/MT/rules.MT.raw.$psi-$si\n";
    if( system("trees2rules $SOURCE $i $ni > $DESTDIR/MT/rules.MT.raw.$psi-$si") ){
	mydie("trees2rules error for $SOURCE between $i and $ni");
    }
    $psi = $si;
}

sub mydie{
    my ($err) = @_;

    my $errlog = "$DESTDIR/errorlog.extractRules";
    open(E, ">$errlog") or die "Cannot open errlog file $errlog\n";
    print E "$err\n";
    close(E);
    die;
}
