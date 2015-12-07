#!/usr/usc/bin/perl
 
# needs a list to extract the nbest specified

open(T, "tmp.list") or die;
while($line=<T>){
	chomp($line);
	$list{$line} = 1;
}
close(T);

while($line=<>){
	if( $line=~ /NBEST sent=(.*?) nbest=0/ ){
	  if( $list{$1} ){
	    print $line;
	  }
	}
}	 
