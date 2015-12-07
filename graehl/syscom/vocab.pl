#!/usr/bin/env perl
use strict;

use File::Basename qw(dirname basename);

BEGIN {
 	push @INC, dirname($0);
}

require "libgraehl.pl";

my $count;
my $index;
my $out;

my @options=(
"write the unique words in the input, one per line",
["count!"=>\$count,"also write count after the word"],
    ["out=s"=>\$out,"write output here"],
#["index=s"=>\$index,"assign words an index starting at this"],
    );


my $cmdline=&escaped_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
info("COMMAND LINE:");
info($cmdline);
show_opts(@opts);

### main program ################################################
outz_stdout($out) if $out;
&argvz;

my %v;

while(<>) {
    $v{$_}++ for (split);
}

while (my ($k,$v)=each %v) {
    print $k;
    print " ",$v if $count;
    print "\n";
}
