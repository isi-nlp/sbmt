#!/usr/usc/bin/perl -w

while(my $line=<>){
    $line =~ /^([0-9]+) (.*?)$/;
    my $cnt = $1;
    my $f = permute($2, \@perm);
    for(my $i=0; $i<$f; $i++){
	print "$cnt $perm[$i]\n";
    }
}

sub permute
{
    my ($line, $perm) = @_;

    if( $line !~ /^(.*?) (.*)$/ ){
	$$perm[0] = $line;
	return 1;
    }
    my $el = $1; 
    my @cperm = ();
    my $cf = permute($2, \@cperm);
    my $f = 0;
    for(my $i=0; $i<$cf; $i++){
	my @cline = split " ", $cperm[$i];
	for(my $j=0; $j<=scalar(@cline); $j++){
	    my @nline = ();
	    for(my $k=0; $k<$j; $k++){ $nline[$k] = $cline[$k]; }
	    $nline[$j] = $el;
	    for(my $k=$j; $k<scalar(@cline); $k++){ $nline[$k+1] = $cline[$k]; }
	    $$perm[$f++] = join " ", @nline;
	}
    }
    return $f;
}
