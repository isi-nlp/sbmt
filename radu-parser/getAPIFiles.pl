#!/usr/usc/bin/perl
 
# take grammar.lex and grammar.nts and produce api.lex and api.nts

$fname = "../GRAMMAR/$ARGV[0]/grammar.lex";
open(F, $fname) or die "Cannot open $fname";

$oname = "../GRAMMAR/$ARGV[0]/api.lex";
open(O, ">$oname") or die "Cannot open $oname";
$UNKINDEX = 65501;
$NAV = 65502;

print O "0 +STOP+ 0\n";
print O "0 +UNK+ $UNKINDEX\n";
print O "0 +N/A+ $NAV\n";
$nwordIndex = 0;
while($line=<F>){
    chomp($line);
    @line = split " ", $line;
    $idx = ++$nwordIndex;
    if( $idx==$UNKINDEX ){ $idx = ++$nwordIndex; }
    if( $idx==$NAV ){ $idx = ++$nwordIndex; }
    print O "$line[0] $line[2] $idx\n";
}
close(O);
close(F);

$fname = "../GRAMMAR/$ARGV[0]/grammar.nts";
open(F, $fname) or die "Cannot open $fname";

$oname = "../GRAMMAR/$ARGV[0]/api.nts";
open(O, ">$oname") or die "Cannot open $oname";
print O "0 +STOP+ 0\n";
$nlabelIndex = 0;
while($line=<F>){
    chomp($line);
    @line = split " ", $line;
    $idx = ++$nlabelIndex;
    print O "$line[1] $line[0] $idx\n";
}
close(O);
close(F);
