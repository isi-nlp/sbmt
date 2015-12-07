#!/usr/usc/bin/perl -w

open(P, "$ARGV[0]") or die;
open(S, "$ARGV[1]") or die;

while(my $line=<P>){
    chomp($line);
    $line =~ /^([0-9]+) (.*?)$/;
    my $score = <S>;
    my $cnt = $1;
    my $f = factorial($cnt);
    my $maxscore = $score; $maxline = $line;
    for(my $i=1; $i<$f; $i++){
	$line=<P>; chomp($line);
	$score = <S>; chomp($score);
	if( $maxscore <= $score ){
	    $maxscore = $score;
	    $maxline = $line;
	}
    }
    $maxline =~ s/ (.*?) / $1_/g;
    print "$maxline\n";
}

sub factorial
{
    my ($n) = @_;

    if( $n<=2 ){ return $n; }
    return $n*factorial($n-1);
}
