#!/usr/usc/bin/perl
 
while($line=<>){
    chomp($line);
    while( $line =~ /~(.*?) (.*?) @@@\{(.*?)\}@@@/g ){
	print "$2 : $3\n";
    }
}
