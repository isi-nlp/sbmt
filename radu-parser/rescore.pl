#!/usr/usc/bin/perl

$RESCORE = 1;
$NR = 10;

@alpha = ( 1, 0, 1, 0);
#@alpha = ( 0.536472, 0.367537, -0.00325837, 2.58861);

open(F0, $ARGV[0]) or die; # HYP 

open(F1, $ARGV[1]) or die; # TM score 
open(F2, $ARGV[2]) or die; # NGLM score
open(F3, $ARGV[3]) or die; # SBLM score
open(F4, $ARGV[4]) or die; # LEN score

$cnt = 0; $nr = 0;
while($hyp=<F0>){ 
    chomp($hyp);
    $cnt++;
    $line1=<F1>; chomp($line1); 
    $sbtm = $line1; if( $sbtm>0 ){ $sbtm = -$sbtm; }
    $line2=<F2>; chomp($line2); 
    $nglm = $line2; if( $nglm>0 ){ $nglm = -$nglm; }
    $line3=<F3>; chomp($line3); 
    $sblm = $line3; if( $sblm =~ /^(.*?) / ){ $sblm = $1; }  if( $sblm>0 ){ $sblm = -$sblm; }
    if( $sblm eq "-Inf" ){ 
	$sblm = -3000; 
    }
    $line4=<F4>; chomp($line4); 
    $len = $line4; if($len>0){ $len = log($len); }

    if( $hyp !~ /^==/ ){
	if( $RESCORE ){
	    $s = $alpha[0]*$sbtm + $alpha[1]*$nglm + $alpha[2]*$sblm + $alpha[3]*$len;
	    $key = "$hyp POS=$cnt"; # POS needed because of multiple same hyp strings in nbest list
	    $score{$key}{"total"} = $s;
	    $score{$key}{"sbtm"} = $sbtm;
	    $score{$key}{"nglm"} = $nglm;
	    $score{$key}{"sblm"} = $sblm;
	    $score{$key}{"len"} = $len;
	}
    }
    else{
	@score = sort{ $score{$b}{"total"} <=> $score{$a}{"total"} }(keys %score);
	$nscore = scalar @score; 
	$score = $score{$score[0]}{"total"};
	$score[0] =~ s/ POS=(.*?)$//;
	for($i=0; $i<$NR; $i++){
	    printf "%.5f %.5f %.5f %s\n", $score, $score{$score[$i]}{"sbtm"}, $score{$score[$i]}{"sblm"}, $score[$i];
	}  
	#printf "%s\n", $score[0];
	%score = ();
	$cnt = 0; $nr++;
	#print STDERR "Writing sentence $nr\n";
	$avgN += $nscore;
	$avgD += 1;  
    }
}
printf STDERR "Average N in n-best: %.2f\n", $avgN/$avgD;
