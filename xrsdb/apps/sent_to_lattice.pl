#!/usr/bin/env perl

use strict;
use Getopt::Long;

my $id = 1;

sub usage {
    print "\n";
    print "sent_to_lattice.pl [--start-id=n] < SENTFILE > LATFILE\n";
    print "help     -- print this message\n";
    print "start-id -- lattice id fields will begin with this value and auto-increment\n";
    exit;
}

GetOptions( "help" => \&usage
          , "start-id:i" => \$id    
          )
          ;
          
while (<>) {
    s/\\/\\\\/g;
    s/"/\\"/g;
    s/^\s+//;
    s/\s+$//;
    my @words = split(/\s+/,$_);
    next if scalar(@words) <= 0;
    print "lattice id=\"$id\" {\n";
    $id++;
    my $lft = 0;
    my $rt = 1;
    foreach my $word (@words) { if ($word ne "") {
        print "    [$lft,$rt] \"$word\" ;\n";
        $lft++;
        $rt++;
    } }
    print "};\n";
}
