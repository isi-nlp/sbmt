#!/usr/usc/bin/perl

# transform input for tokenization from MT style to PTB style

while($line = <>){
    chomp($line);
    $line =~ s/ \@\-\@ /-/g;
    $line =~ s/\(/-LRB-/g;    
    $line =~ s/\)/-RRB-/g;
    $line =~ s/\{/-LCB-/g;    
    $line =~ s/\}/-RCB-/g;
    $line =~ s/\" (.*?) \"/`` $1 ''/g;
    $line =~ s/\"/''/g;
    $line =~ s/^\* /-- /;
    print "$line\n";
}
