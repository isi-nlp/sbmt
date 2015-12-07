#!/usr/bin/env perl
# echo `systems.pl a=aoutput dir/b.output` => A=aoutput B=dir/b.output
# your filenames get translated to (uppercased) SYSNAME=filename
# where filename is [dirs/]SYSNAME.xxx,
# or SYSNAME=xxx (i.e. this is idempotent)

for (@ARGV) {
    if (m#^([^/:.]+?)=(.*)#) {
        print STDERR "$1\n";
        print uc($1),"=$2 ";
    } elsif (m#^(.*/)?([^./]+)(.*)#) {
        print STDERR "$2\n";
        print uc($2),"=$_ ";
    } else {
        die "unknown system for $_";
    }
}
print "\n";
