#!/usr/usc/bin/perl

# takes either only 
# % mergeRules.pl headframe hf [and considers the TRAINING/MT/ files, or
# % mergeRules.pl headframe hf file 1 file 2

if( scalar(@ARGV)==2 ){
    $SOURCE = "TRAINING/MT/rules.MT.sort";
    $SOURCE .= ".$ARGV[0].*";
}
else{
    $SOURCE = "$ARGV[2] $ARGV[3]";
} 


$SIDE = $ARGV[1];

$nrules = 0;
$notPurged = 1;
open(F, "ls $SOURCE |");
while($line=<F>){
    chomp($line);
    open(G, $line) or die "Cannot open $line\n";
    printf stderr "Visiting $line\n";
    while($rule=<G>){
	chomp($rule);
	$rule =~ /^( *)(.*?)[\cI| ](.*)$/;
	$cnt = $2; $rule = $3;
	if( $rule !~ /^\+$SIDE\+ / ){ next; }
	if( $hash{$rule} ){
	    $hash{$rule} += $cnt;
	}
	else{
	    $hash{$rule} = $cnt;
	    $nrules++;
	    if( $nrules%100000==0 ){
		printf STDERR "# rules: $nrules\n";
	    }
	}
    }
    close(G);
}
close(F);

@rules = (keys %hash);
printf STDERR "Writing out %d rules...\n", $nrules;
for($i=0; $i<=$#rules; $i++){
    print "$hash{$rules[$i]} $rules[$i]\n";
}
print STDERR "Done\n";
