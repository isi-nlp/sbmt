#!/usr/usc/bin/perl

if( scalar(@ARGV)!=5 ){
    die "Usage: intersectHFwVoc.pl voc-file grammar.lex ThCnt rules.hf DestDir\n";
}

open(F, $ARGV[0]) or die "Cannot open vocabulary file\n";
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

open(F, $ARGV[1]) or die "Cannot open lex file\n";
while($line=<F>){
    chomp($line);
    @line = split " ", $line;
    $lex{$line[2]} = $line[0];
}
close(F);

$ThCnt = $ARGV[2];
$lex{"+UNK+"} = $ThCnt;

if( $ARGV[3] =~ /^(.*?)\/rules.(.*?).headframe.joint=2$/ ){
    $rtype = $2;
}
else{ die "Could not extract type from $ARGV[3]\n"; }
$DestDir = $ARGV[4];
if( $DestDir !~ /\/$/ ){ $DestDir .= "/"; }
$eventsPrefix = "events.";
$out = $DestDir.$eventsPrefix.$rtype.".headframe.$voc";
$out2 = $DestDir.$eventsPrefix.$rtype.".headframe.ptl.$voc";
$out3 = $DestDir.$eventsPrefix.$rtype.".headframe.prior.$voc";

open(F, $ARGV[3]) or die "Cannot open training HF file\n";

open(O, ">$out") or die "Cannot open output HF file $out\n";
open(O2, ">$out2") or die "Cannot open output HF file $out2\n";
open(O3, ">$out3") or die "Cannot open output HF file $out3\n";

while($line=<F>){
    chomp($line);
    if( $line =~ /([0-9]+) (.*?) (.*?) (.*?) (.*?) \-\> (.*?) \{(.*?)\} \{(.*?)\}$/ ){
	$cnt = $1; $flag = $2; $P = $3; $t = $4; $w = $5; $H = $6; $CL = $7; $CR = $8; 
	
	# HEAD
	# head-1
	$w1 = $w; $ignore = 0;
	if( $lex{$w} < $ThCnt ){ $w1 = "+UNK+"; }
	$joint = "+head-1+ $P $t $w1 -> $H";
	 $cond = "+head-1+ $P $t $w1";
	if( $w1 eq "+UNK+" || $voc{$w} ){
	    record($joint, $cond, $cnt);
	}

	# TOP
	# top-1
	if( $P eq "S1" ){
	    $w1 = $w; $ignore = 0;
	    if( $lex{$w} < $ThCnt ){ $w1 = "+UNK+"; }
	    $joint = "+top-1+ $P $H $t -> $w1";
	     $cond = "+top-1+ $P $H $t";
	    if( !$voc{$w} && $lex{$w} >= $ThCnt ){ $ignore = 1; }
	    record($joint, $cond, $cnt, $ignore);
	}

	# FRAME
	# frameL-1
	$w1 = $w; $ignore = 0;
	if( $lex{$w} < $ThCnt ){ $w1 = "+UNK+"; }
	$joint = "+frame-1+ +l+ $P $H $t $w1 -> {$CL}";
	 $cond = "+frame-1+ +l+ $P $H $t $w1";
	if( $w1 eq "+UNK+" || $voc{$w} ){
	    record($joint, $cond, $cnt);
	}
	# frameR-1
	$joint = "+frame-1+ +r+ $P $H $t $w1 -> {$CR}";
	 $cond = "+frame-1+ +r+ $P $H $t $w1";
	if( $w1 eq "+UNK+" || $voc{$w} ){
	    record($joint, $cond, $cnt);
	}

	# PT & LEX & Prior
	if( $H eq $t ){
	    # these are counts to be transferred to intersectMODwVoc.pl; no +UNK+ modification is performed 
	    $joint = "+pt+ $t -> $w";
	     $cond = "+pt+ $t";
	    record($joint, $cond, $cnt);
	    $joint = "+pt+ +LEX+ -> $w";
	     $cond = "+pt+ +LEX+";
	    record($joint, $cond, $cnt);
	}
	# these are counts to be transferred to intersectMODwVoc.pl; only vocabulary words are needed
	$w1 = $w;
	if( $lex{$w} < $ThCnt ){ $w1 = "+UNK+"; }
	if( $w1 eq "+UNK+" || $voc{$w} ){ 
	    $joint = "+rior-0+ +TW+ -> $t $w1";
	}
	else{ $joint = "+rior-0+ +TW+ -> $t +N/A+"; }
	 $cond = "+rior-0+ +TW+";
	record($joint, $cond, $cnt);

	$joint = "+rior-1+ $t $w1 -> $H"; 
	 $cond = "+rior-1+ $t $w1";
	if( $w1 eq "+UNK+" || $voc{$w} ){ 	    
	    record($joint, $cond, $cnt);	
	}
    }
    else{ print STDERR "Error in this line: $line\n" and die; }
}
close(F);

foreach $key (keys %jointCnt){
    $ckey = $key;
    $jcnt = $jointCnt{$ckey}[0];
    $key =~ s/ \-\> (.*?)$//;
    $ccnt = $condCnt{$key};
    $ucnt = $uCnt{$key};
    @key = split " ", $key;
    if( $key[0] eq "+pt+" ){
	print O2 "$ckey | $jcnt\n";
    }
    elsif( $key[0] =~ /\+rior/ ){
	print O3 "$ckey | $jcnt\n";
    }
    else{
	if( $jointCnt{$ckey}[1] ){
	    if( !$markedCond{$key} ){
		print O "$key -> +N/A+ | 0 $ccnt $ucnt\n";
		$markedCond{$key} = 1;
	    }
	}
	else{ print O "$ckey | $jcnt $ccnt $ucnt\n"; }
    }
}
close(O);
close(O2);
close(O3);

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
    $condCnt{$cond} += $cnt;
    if( $newevent ){
	$uCnt{$cond} += 1;
    }
}
