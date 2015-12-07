#!/usr/bin/perl -w

# given a grammar.lex and a POSed corpus, report the UNK rate

my %voc = ();
open(G, "$ARGV[0]") or die;
while(my $line=<G>){
    chomp($line);
    my @line = split " ", $line;
    $voc{$line[2]} = $line[0];
}
close(G);
$voc{"."} = 1; $voc{"!"} = 1; $voc{"?"} = 1;
$voc{"\""} = 1; $voc{"''"} = 1; $voc{"``"} = 1;

my ($unk, $total) = (0, 0);
open(F, "$ARGV[1]") or die;
while(my $line=<F>){
    chomp($line);
    my @line = split " ", $line;
    for($i=1; $i<=$#line; $i+=2){
	if( !$voc{$line[$i]} ){
	    $unk++;
	    print "$line[$i]\n";
	}
	$total++;
    }
}
close(F);
printf STDERR "Unknown word rate: %.2f percent (%d out of %d)\n",($unk/$total)*100, $unk, $total;
