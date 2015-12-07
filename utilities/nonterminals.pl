#!/usr/bin/env perl
# reads a grammar file, stores the name of all non-terminals on
my %ntset;
while(<>) {
    my $line = $_;
    if ($line =~ /^(([^( ]+)\(.*\)) -> .* ### .*/o) {
        if ($2 =~ /^\s*$/) { print STDERR $line;}
        $ntset{$2} = "";
        my $lhs = $1;
        my $s = 0;
        foreach my $m ($lhs =~ /x\d+:([^\s\)]+)/g) {
            #print STDERR "\"$1\" ";
            if ($1 =~ /^\s*$/) { $s = 1;}
            $ntset{$1}="";
        }
        #print STDERR "--> $2\n";
        if ($s) { print STDERR $line, "\n"; }
        
    }
}

foreach my $nt (keys(%ntset)) {
    print "$nt \n"; 
}
