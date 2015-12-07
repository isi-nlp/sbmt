#!/usr/bin/env perl

#graehl@isi.edu
#binmode ARGVOUT;

use strict;
use warnings;
use Getopt::Long;
use File::Basename;

my $blobbase="/home/hpc-22/dmarcu/nlg/blobs";

### script info ##################################################
my $scriptdir;                  # location of script
my $scriptname;                 # filename of script
my $BLOBS;

BEGIN {
    $scriptdir = &File::Basename::dirname($0);
    ($scriptname) = &File::Basename::fileparse($0);
    push @INC, $scriptdir;      # now you can say: require "blah.pl" from the current directory
    $ENV{BLOBS}='/home/hpc-22/dmarcu/nlg/blobs' unless exists $ENV{BLOBS};
    $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
    $BLOBS=$ENV{BLOBS};
    my $libgraehl="$BLOBS/libgraehl/unstable";
    push @INC,$libgraehl if -d $libgraehl;
}

require "libgraehl.pl";

### arguments ####################################################

use Pod::Usage;
my $count_only = 0;
my $INF=999999999;
my $minchars =  1;
my $maxchars = $INF;
my $chunksize = 1;
my $nchunks = $INF;
my $offset = 0;
my $interleave = 1;
my $alast=0;

GetOptions("help" => \&usage,
           "lowerlen:i" => \$minchars,
           "upperlen:i" => \$maxchars,
           "sizechunks:i" => \$chunksize,
           "nchunks:i" => \$nchunks,
           "offset:i" => \$offset,
           "interleave:i" => \$interleave,
           "count" => \$count_only,
           "alwayslast" => \$alast,
           );

#&debug($chunksize);
my %OPT;
my $optc=0;
my $t=0;
while(0) {
#while (defined($ARGV[0]) && $ARGV[0] =~ /^\-(.+)$/) {
    shift;
    &usage if ($1 eq 'h');
    if ($1 eq 'c') {
	$count_only = 1;
    }else {
	$OPT{$1} = shift;
    }
    ++$optc;
}
#&usage unless $optc > 0;


my $chunk=0;
my $lineno=0;
my $achunk = 0;

my $prevblank = 1;

my $active = ($offset == 0);
my $lines;
my $last;

&argvz;
while(<>) {
    next if (length($_) < $minchars || length($_) > $maxchars);
    ++$lineno;
    if ($active && !$count_only) {
        print;
        $last=undef;
    } else {
        $last=$_;
    }
    if ($lineno >= $chunksize) {
        $lineno = 0;
        if ( $offset > 0) {
            --$offset;
            $active = ($offset==0);
        } else {
            ++$chunk;
            ++$achunk if $active;
            $active=($chunk % $interleave == 0);
            last if $achunk >= $nchunks;
        }
    }
}

print $last if defined($last) && $alast;

print $achunk,"\n" if $count_only;


sub usage {
print <<'EOF';
Text selector

'sample_lines.pl [-l minchars] [-u maxchars] [-s chunksize] [-o offset] [-n nchunks] [-i interleave] [-c] [-a] [input_file ...]'
defaults: minchars=0, maxchars=+INF, chunksize=1, nchunks=+INF, interleave=1, offset=0

For all the [input_file ...] (or stdin if none), copy lines, ignoring those that
are less than 'minchars' chars or more than maxchars (counting whitespace and
newline characters).  Ignore the first 'offset' chunks (of 'chunksize' lines),
then repeat this 'nchunks' times (or until input is exhausted): output the next
chunk and skip 'interleave'-1 chunks.  If -c is specified, do not output the
lines, but rather, a count of how many would have been output.  If -a is set,
always print the last line (once) - this option makes corpus partitioning with
different offsets impossible, though.

For example:
$ sample_lines.pl -s 5 -i 2 -o 0 lines > line.odd
$ sample_lines.pl -s 5 -i 2 -o 1 lines > line.even

would partition the lines  into two files of alternating 5 line chunks (the first into line.odd, the second into line.even, and so on)


EOF
exit -1;
}
