#!/usr/usc/bin/perl

for($i=0; $i<scalar(@ARGV); $i++){
    $ARGV[$i] =~ /(.*)\.(.*?)\.gz/;
    $hash{$ARGV[$i]} = $2;
}
@list = sort { $hash{$a} <=> $hash{$b} }(keys %hash);

for($i=0; $i<scalar(@list); $i++){
    open(F, "zcat $list[$i] |") or die "Cannot open $list[$i]\n";
    $flag = 0;
    while($line=<F>){
	if( $line =~ /NBEST/ && $flag==0 ){
	    $flag = 1;
	    $line =~ /hyp={{{(.*?)}}}/;
	    print "$1\n";
	    last;
	}
	elsif( $line =~ /RECOVERED/ && $flag==0 ){
	    $line =~ /hyp={{{(.*?)}}}/;
	    print "$1\n";
	    last;
	}
    }
    if( $flag ){ $ok++; }
    else{ $rec++; }
}
print STDERR "Decoded SBTM: $ok\n";
print STDERR "Decoded recover: $rec\n";
