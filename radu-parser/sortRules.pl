#!/usr/usc/bin/perl

if( $ARGV[0] eq "hf" ){ $flag = ""; $name = "headframe"; }
else{ $flag = "-v"; $name = "mod"; }

$SOURCEDIR = $ARGV[1];
$STEP = 200000;

open(F, "ls $SOURCEDIR/MT |") or die "Cannot list $SOURCEDIR/MT\n";
while($line=<F>){
    chomp($line);
    $out = $line;
    if( $out =~ s/\.raw\./.sort./ ){
	print  "cat $line | grep $flag '^+hf+ ' | sort | uniq -c | sort -nr > $out\n";
	system("cat $line | grep $flag '^+hf+ ' | sort | uniq -c | sort -nr > $out");
    }
    else{ die "Name convention not met: $out\n"; }
}


