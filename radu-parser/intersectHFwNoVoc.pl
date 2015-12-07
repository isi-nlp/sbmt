#!/usr/usc/bin/perl

if( scalar(@ARGV)!=2 ){
    die "Usage: intersectHFwNoVoc.pl rules.hf DestDir\n";
}

if( $ARGV[0] =~ /^(.*?)\/rules.(.*?).headframe.joint=2$/ ){
    $rtype = $2;
}
else{ die "Could not extract type from $ARGV[0]\n"; }
$DestDir = $ARGV[1];
if( $DestDir !~ /\/$/ ){ $DestDir .= "/"; }
$eventsPrefix = "events.";
$out = $DestDir.$eventsPrefix.$rtype.".headframe.nvo";
$out2 = $DestDir.$eventsPrefix.$rtype.".headframe.prior.nvo";

open(F, $ARGV[0]) or die "Cannot open training HF file\n";;

open(O, ">$out") or die "Cannot open output HF file $out\n";
open(O2, ">$out2") or die "Cannot open output HF file $out\n";

while($line=<F>){
    chomp($line);
    if( $line =~ /([0-9]+) (.*?) (.*?) (.*?) (.*?) \-\> (.*?) \{(.*?)\} \{(.*?)\}$/ ){
	$cnt = $1; $flag = $2; $P = $3; $t = $4; $w = $5; $H = $6; $CL = $7; $CR = $8; 
	
	# HEAD
	# head-2
	$joint = "+head-2+ $P $t -> $H";
	 $cond = "+head-2+ $P $t";
	record($joint, $cond, $cnt);
	# head-3
	$joint = "+head-3+ $P -> $H";
	 $cond = "+head-3+ $P";
	record($joint, $cond, $cnt);
	# head-4
	$joint = "+head-4+ +HEAD+ -> $H";
	 $cond = "+head-4+ +HEAD+";
	record($joint, $cond, $cnt);

	# TOP
	# top-0
	if( $P eq "S1" ){
	    $joint = "+top-0+ $P -> $H $t";
	     $cond = "+top-0+ $P";
	    record($joint, $cond, $cnt);
	}

	# FRAME
	# frameL-2
	$joint = "+frame-2+ +l+ $P $H $t -> {$CL}";
	 $cond = "+frame-2+ +l+ $P $H $t";
	record($joint, $cond, $cnt);
	# frameL-3
	$joint = "+frame-3+ +l+ $P $H -> {$CL}";
	 $cond = "+frame-3+ +l+ $P $H";
	record($joint, $cond, $cnt);
	# frameL-4
	$joint = "+frame-4+ +l+ +FRAME+ -> {$CL}";
	 $cond = "+frame-4+ +l+ +FRAME+";
        record($joint, $cond, $cnt);

	# frameR-2
	$joint = "+frame-2+ +r+ $P $H $t -> {$CR}";
	 $cond = "+frame-2+ +r+ $P $H $t";
	record($joint, $cond, $cnt);
	# frameR-3
	$joint = "+frame-3+ +r+ $P $H -> {$CR}";
	 $cond = "+frame-3+ +r+ $P $H";
	record($joint, $cond, $cnt);
	# frameR-4
	$joint = "+frame-4+ +r+ +FRAME+ -> {$CR}";
	 $cond = "+frame-4+ +r+ +FRAME+";
        record($joint, $cond, $cnt);

	# PRIOR
	# prior-2
	$joint = "+rior-2+ $t -> $H";
	 $cond = "+rior-2+ $t";
	record($joint, $cond, $cnt);
	# prior-3
	$joint = "+rior-3+ +LBL+ -> $H";
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
    @key = split " ", $key;
    if( $key[0] =~ /\+rior/ ){
	print O2 "$ckey | $jcnt\n";
    }
    else{
	print O "$ckey | $jcnt $ccnt $ucnt\n";
    }
}
close(O);
close(O2);

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
