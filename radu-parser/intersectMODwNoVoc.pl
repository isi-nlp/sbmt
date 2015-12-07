#!/usr/usc/bin/perl

if( scalar(@ARGV)!=2 ){
    die "Usage: intersectMODwNoVoc.pl rules.mod DestDir\n";
}

if( $ARGV[0] =~ /^(.*?)\/rules.(.*?).mod.joint=2$/ ){
    $rtype = $2;
}
else{ die "Could not extract type from $ARGV[0]\n"; }
$DestDir = $ARGV[1];
if( $DestDir !~ /\/$/ ){ $DestDir .= "/"; }
$eventsPrefix = "events.";
$out = $DestDir.$eventsPrefix.$rtype.".mod.nvo";

open(F, $ARGV[0]) or die "Cannot open training MOD file $ARGV[0]\n";
open(O, ">$out") or die "Cannot open output MOD file $out\n";

$fname = $out;
$fname =~ s/\.mod\./.headframe.prior./;
open(F2, $fname) or die "Cannot open prior file $fname\n";
while($line=<F2>){
    chomp($line);
    @line = split " ", $line;
    $type = $line[0]; $t = $line[1]; $L = $line[3]; $cnt = $line[5];
    $joint = "$type $t -> $L";
     $cond = "$type $t";
    record($joint, $cond, $cnt);
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
	
	# MOD
	# mod1-2
	$joint = "+mod1-2+ $flag $P {$C} $V $T $H $t -> $M $mt $cc $punc";
	 $cond = "+mod1-2+ $flag $P {$C} $V $T $H $t";
	record($joint, $cond, $cnt);
	# mod1-3
	$joint = "+mod1-3+ $flag $P {$C} $V $T $H -> $M $mt $cc $punc";
	 $cond = "+mod1-3+ $flag $P {$C} $V $T $H";
	record($joint, $cond, $cnt);
	# mod1-4
	$joint = "+mod1-4+ $flag +MOD1+ -> $M $mt $cc $punc";
	 $cond = "+mod1-4+ $flag +MOD1+";
	record($joint, $cond, $cnt);

	# CP
	for($i=0; $i<=1; $i++){
	    if( $i==0 ){ $cp = $cc; $cpt = $ct; $type = "+CC+";}
	    else{ $cp = $punc; $cpt = $pt; $type = "+PUNC+";}
	    if( $cp ){
		# cc1-2
		$joint = "+cp1-2+ $P $H $t $M $mt $type -> $cpt";
		 $cond = "+cp1-2+ $P $H $t $M $mt $type";
		record($joint, $cond, $cnt);
		# cc1-3
		$joint = "+cp1-3+ $type -> $cpt";
		 $cond = "+cp1-3+ $type";
		record($joint, $cond, $cnt);
	    }
	}

	if( $M eq "+STOP+" ){ next; }

	# PRIOR
	# prior-2
	$joint = "+rior-2+ $mt -> $M";
	 $cond = "+rior-2+ $mt";
	record($joint, $cond, $cnt);
	# prior-3
	$joint = "+rior-3+ +LBL+ -> $M";
	 $cond = "+rior-3+ +LBL+";
	record($joint, $cond, $cnt);	
    }
    else{ print STDERR "Error in this line: $line\n" and die; }
}
close(F);

foreach $key (keys %jointCnt){
    $jcnt = $jointCnt{$key};
    $ckey = $key;
    $key =~ s/ \-\> (.*?)$//;
    $ccnt = $condCnt{$key};
    $ucnt = $uCnt{$key};
    print O "$ckey | $jcnt $ccnt $ucnt\n";
}

sub record{
    my ($joint, $cond, $cnt) = @_;

    $newevent = 0;
    if( $jointCnt{$joint} ){
	$jointCnt{$joint} += $cnt;
    }
    else{ 
	$jointCnt{$joint} = $cnt;
	$newevent = 1;
    }
    $condCnt{$cond} += $cnt;
    if( $newevent ){
	$uCnt{$cond} += 1;
    }
}
