#!/usr/usc/bin/perl

if( scalar(@ARGV)!=5 ){
    die "Usage: intersectMODwVoc.pl grammar.voc grammar.lex ThCnt rules.mod DestDir\n";
}

open(F, $ARGV[0]) or die "Cannot open vocabulary file $ARGV[0]\n";
while($line=<F>){
    chomp($line);
    @line = split " ", $line;
    for($i=0; $i<=$#line; $i++){
	$voc{$line[$i]} = 1;
    }
}
close(F);
$ARGV[0] =~ /^(.*)\/(.*?)$/;
$voc = $2;

open(F, $ARGV[1]) or die "Cannot open lex file $ARGV[1]\n";
while($line=<F>){
    chomp($line);
    @line = split " ", $line;
    $lex{$line[2]} = $line[0];
}
close(F);

$ThCnt = $ARGV[2];
$lex{"+UNK+"} = $ThCnt;

if( $ARGV[3] =~ /^(.*?)\/rules.(.*?).mod.joint=2$/ ){
    $rtype = $2;
}
else{ die "Could not extract type from $ARGV[3]\n"; }
$DestDir = $ARGV[4];
if( $DestDir !~ /\/$/ ){ $DestDir .= "/"; }
$eventsPrefix = "events.";
$out = $DestDir.$eventsPrefix.$rtype.".pt.$voc";

open(F, $ARGV[3]) or die "Cannot open training MOD file $ARGV[3]\n";
open(O, ">$out") or die "Cannot open output MOD file $out\n";

$fname = $out;
$fname =~ s/\.mod\./.headframe./;
open(F2, $fname) or die "Cannot open voc-driven ptl file $fname\n";
while($line=<F2>){
    chomp($line);
    @line = split " ", $line;
    $t = $line[1];
    $w = $line[3];
    $cnt = $line[5];
    $w1 = $w; $ignore = 0;
    if( $lex{$w} < $ThCnt ){ $w1 = "+UNK+"; }
    $joint = "+pt+ $t -> $w1";
     $cond = "+pt+ $t";
    if( !$voc{$w} && $lex{$w} >= $ThCnt ){ $ignore = 1; }
    record($joint, $cond, $cnt, $ignore);
}
close(F2);

while($line=<F>){
    chomp($line);
    if( $line =~ /([0-9]+) (.*?) (.*?) (.*?) (.*?) (.*?) \{(.*?)\} (.*?) (.*?) \-\> (.*?) (.*?) (.*?) (.*?)$/ ){
	$cnt = $1; $flag = $2; $P = $3; $H = $4; $t = $5; $w = $6; $C = $7; $V = $8; $T = $9; $M = $10; $mt = $11; $mw = $12; $rest = $13;
	@rest = split " ", $rest;
	if( $rest[0] eq "0" ){ $cc = 0; $ct = 0; $cw = 0; }
	else{ $cc = 1; $ct = $rest[1]; $cw = $rest[2]; }
	if( $cc==0 ){
	    if( $rest[1] eq "0" ){ $punc = 0; $pt = 0; $pw = 0; }
	    else{ $punc = 1; $pt = $rest[2]; $pw = $rest[3]; }
	}
	else{
	    if( $rest[3] eq "0" ){ $punc = 0; $pt = 0; $pw = 0; }
	    else{ $punc = 1; $pt = $rest[4]; $pw = $rest[5]; }
	}
	
	# PT & LEX
	if( $M ne "+STOP+" ){
	    for($i=0; $i<=2; $i++){
		if( $i==0 ){ $cp = ($M eq $mt); $t1 = $mt; $w1 = $mw; }
		elsif( $i==1 ){ $cp = $cc; $t1 = $ct; $w1 = $cw; }
		else{ $cp = $punc; $t1 = $pt; $w1 = $pw; }
		if( $cp ){
		    $w2 = $w1; $ignore = 0;
		    if( $lex{$w1} < $ThCnt ){ $w2 = "+UNK+"; }
		    $joint = "+pt+ $t1 -> $w2";
		     $cond = "+pt+ $t1";
		    if( !$voc{$w1} && $lex{$w1} >= $ThCnt ){ $ignore = 1; }
		    record($joint, $cond, $cnt, $ignore);
		    $joint = "+pt+ +LEX+ -> $w2";
		     $cond = "+pt+ +LEX+";
		    record($joint, $cond, $cnt, $ignore);
		}
	    }
	}
    }
    else{ print STDERR "Error in this line: $line\n" and die; }
}
close(F);

foreach $key (keys %jointCnt){
    $ckey = $key;
    $jcnt = $jointCnt{$ckey}[0];
    $key =~ s/ \-\> (.*?)$//;
    $ccnt = $condCnt{$key}[0];
    $ucnt = $condCnt{$key}[1];
    if( $jointCnt{$ckey}[1] ){
	if( !$markedCond{$key} ){
	    print O "$key -> +N/A+ | 0 $ccnt $ucnt\n";
	    $markedCond{$key} = 1;
	}
    }
    else{
	print O "$ckey | $jcnt $ccnt $ucnt\n";
    }
}

sub record{
    my ($joint, $cond, $cnt, $ignore) = @_;

    $newevent = 0;
    if( $jointCnt{$joint} ){
	$jointCnt{$joint}[0] += $cnt;
    }
    else{ 
	$jointCnt{$joint}[0] = $cnt;
	$jointCnt{$joint}[1] = $ignore;
	$newevent = 1;
    }

    $condCnt{$cond}[0] += $cnt;
    if( $newevent ){
	$condCnt{$cond}[1] += 1;
    }
}
