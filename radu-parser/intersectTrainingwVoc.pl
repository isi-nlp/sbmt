#!/usr/usc/bin/perl

my $sourcedir = "/auto/hpc-22/dmarcu/nlg/summer04/resources/radu-parser/radu-parser-v4.0/";

$gramDir = "/auto/hpc-22/dmarcu/nlg/summer04/resources/radu-parser/GRAMMAR/";
$rulesDir = "/auto/hpc-22/dmarcu/nlg/summer04/resources/radu-parser/TRAINING/";
$rulesPrefix = "rules.";
$rulesSuffix = ".joint=2";
$eventsPrefix = "events.";

if( scalar(@ARGV)!=4 ){
    die "Usage: intersectTrainingwVoc.pl vocabulary Type:PTB|MT ThCnt dest\n";
}
$ARGV[0] =~ /^(.*)\/(.*?)$/;
$voc = "$2";

$grammar = $gramDir.$ARGV[1]."/grammar.lex";

$DestDir = $ARGV[3];
if( $DestDir !~ /\/$/ ){ $DestDir .= "/"; }

$eventsPrefName = $DestDir.$eventsPrefix.$ARGV[1];
$all = $eventsPrefName.".allpruned.$voc";
if( -e $all ){
    die "File $all already present\n"; 
}

$rulesHF = $rulesDir.$rulesPrefix.$ARGV[1].".headframe".$rulesSuffix;
$rulesMOD = $rulesDir.$rulesPrefix.$ARGV[1].".mod".$rulesSuffix;

printf STDERR "Extracting nolex events from file $rulesHF\n";
if( !(-e $eventsPrefName.".headframe.nvo" ) ){
    system("perl $sourcedir/intersectHFwNoVoc.pl $rulesHF $DestDir");
}
else{ printf STDERR "File $eventsPrefName.headframe.nvo already present; moving forward...\n"; }

printf STDERR "Intersecting voc file with file $rulesHF\n";
system("perl $sourcedir/intersectHFwVoc.pl $ARGV[0] $grammar $ARGV[2] $rulesHF $DestDir");

printf STDERR "Extracting nolex events from file $rulesMOD\n";
if( !(-e $eventsPrefName.".mod.nvo" ) ){
    system("perl $sourcedir/intersectMODwNoVoc.pl $rulesMOD $DestDir");
}
else{ printf STDERR "File $eventsPrefName.mod.nvo already present; moving forward...\n"; }

printf STDERR "Intersecting voc file with file $rulesMOD\n";
system("perl $sourcedir/intersectMODwVoc-pt.pl $ARGV[0] $grammar $ARGV[2] $rulesMOD $DestDir");
system("perl $sourcedir/intersectMODwVoc-prior.pl $ARGV[0] $grammar $ARGV[2] $rulesMOD $DestDir");
system("perl $sourcedir/intersectMODwVoc-mod1.pl $ARGV[0] $grammar $ARGV[2] $rulesMOD $DestDir");
system("perl $sourcedir/intersectMODwVoc-mod2-1.pl $ARGV[0] $grammar $ARGV[2] $rulesMOD +l+ $DestDir");
system("perl $sourcedir/intersectMODwVoc-mod2-1.pl $ARGV[0] $grammar $ARGV[2] $rulesMOD +r+ $DestDir");
system("perl $sourcedir/intersectMODwVoc-mod2-2.pl $ARGV[0] $grammar $ARGV[2] $rulesMOD $DestDir");
system("perl $sourcedir/intersectMODwVoc-cp.pl $ARGV[0] $grammar $ARGV[2] $rulesMOD $DestDir");

if( !(-e $eventsPrefName.".headframe.nvo" ) ){ die "Missing $eventsPrefName.headframe.nvo\n"; }
if( !(-e $eventsPrefName.".mod.nvo" ) ){ die "Missing $eventsPrefName.mod.nvo\n"; }
if( !(-e $eventsPrefName.".prior.$voc" ) ){ die "Missing $eventsPrefName.prior.$voc\n"; }
if( !(-e $eventsPrefName.".pt.$voc" ) ){ die "Missing $eventsPrefName.pt.$voc\n"; }
if( !(-e $eventsPrefName.".mod1.$voc" ) ){ die "Missing $eventsPrefName.mod1.$voc\n"; }
if( !(-e $eventsPrefName.".mod2-1+l+.$voc" ) ){ die "Missing $eventsPrefName.mod2-1+l+.$voc\n"; }
if( !(-e $eventsPrefName.".mod2-1+r+.$voc" ) ){ die "Missing $eventsPrefName.mod2-1+r+.$voc\n"; }
if( !(-e $eventsPrefName.".mod.cp.$voc" ) ){ die "Missing $eventsPrefName.mod.cp.$voc\n"; }

system("cat $eventsPrefName.headframe.nvo $eventsPrefName.mod.nvo $eventsPrefName.prior.$voc $eventsPrefName.pt.$voc $eventsPrefName.mod1.$voc $eventsPrefName.mod2-1+l+.$voc $eventsPrefName.mod2-1+r+.$voc $eventsPrefName.mod.cp.$voc > $all");

system("rm $eventsPrefName.headframe*.$voc $eventsPrefName.mod*.$voc $eventsPrefName.prior.$voc $eventsPrefName.pt.$voc");

printf STDERR "Done.\n";
