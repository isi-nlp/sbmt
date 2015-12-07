#!/usr/bin/env perl
#
# Adds missingWord feature. This perl works on utf-8 characters
# 2008-05-05 JSV
#
use v5.8.8;			# must be UTF-8 correct for Han !!!
use strict;

BEGIN {
    # use very early - before loading most modules
    delete $ENV{LANG};
    $ENV{'LC_ALL'} = 'C';
}

use utf8;
use Encode; 
use File::Spec;
use File::Basename qw(basename dirname);
use Getopt::Long qw(:config bundling no_ignore_case);
use Carp;

use lib dirname($0);
use NLPTools qw(:all);
use NLPRules qw(:all);

use constant NULL => 'UNKNOWN_WORD';

# global
$main::verbose = 0;

sub usage {
    my $prg = basename($0,'.pl');
    print << "EOF";
Usage: $prg [options] --swlex lexicon[.gz] [ rules[.gz] ]

Optional arguments:
 -v|--verbose             Increase verbosity level.
 -o|--output fn[.gz]      Put output into fn instead of STDOUT.
 --split                  If given, split chi into sub-tokens for spuriousWord.

Mandatory arguments:
 -s|--swlex fn[.gz|.bz2]  spurious word bilexicon.
 rules[.gz]               input grammar, defaults to STDIN.

EOF
    exit 1;
}

sub load_bilex(\%$) {
    # purpose: load a possibly compress t-table into a hash
    # paramtr: %ttable (OUT): new t-table to put things into
    #          $fn (IN): name of the file to load t-table from
    # warning: levels are reversed for missing and spurious word
    # returns: number of entries in bilex
    #
    my $tref = shift;
    my $fn = shift;
    local(*T);
    my $start = time();

    # clean slate
    %{$tref} = ();

    if ( substr($fn,-4) eq '.bz2' ) {
	die "FATAL: Unable to read $fn\n" unless -f $fn;
	open( T, "bzip2 -cd $fn|" ) || die "FATAL: bzip2 failed: $!\n";
    } elsif ( substr($fn,-3) eq '.gz' ) {
	die "FATAL: Unable to read $fn\n" unless -f $fn;
	open( T, "gzip -cd $fn|" ) || die "FATAL: gzip failed: $!\n";
    } else {
	open( T, "<$fn" ) || die "FATAL: open $fn: $!\n";
    }

    # some lexica contain invalid UTF-8
    binmode( T, ':raw' );	

    my $mask = 0xFFFFF;
    my $n = 0;
    my (@x,$y1,$y2);
    while ( <T> ) {
	chomp ;
	footprint(commas($.)) if ( $main::verbose && ( $. & $mask ) == 0 );
	# swFF:     count   trg   src
	# mwFF:     count   src   trg
	if ( (@x = split()) ) {
	    eval {
		$tref->{ Encode::decode('UTF-8',$x[1],1) }{ Encode::decode('UTF-8',$x[2],1) } = $x[0];
		$n++;
	    };
	    if ( $@ ) {
		$@ =~ s/ at .*$//;
		chomp($@);
		warn( "Info: Line ", commas($.), ": Invalid UTF-8 [$@], skipping\n" );
	    }
	} else {
	    warn( "Info: Line ", commas($.), ": Bad line, skipping\n" );
	}
    }
    footprint(commas($.)) if ( $main::verbose && ( $. & $mask ) );
    pageinfo ;
    close T || die "FATAL: close $fn: $!\n";

    # done
    debug( commas($n), ' pairs extracted in ', time()-$start, ' s' );
    $n;
}

sub permute($) {
    # purpose: abcd -> a ab abc abcd b bc bcd c cd d
    my $token = shift;
    my @result = ( $token );
    my @x = ();
    push( @x, $1 ) while ( $token =~ /(\p{Han})/g );
    if ( @x ) {
	# partial or no English, rip apart
	for ( my $i=0; $i < @x; ++$i ) {
	    for ( my $j=$i; $j < @x; ++$j ) {
		push( @result, join( '', @x[$i..$j]) );
	    }
	}
    }

    @result;
}

#
# --- main ------------------------------------------------------
#
debug( "$0 @ARGV" );

binmode( STDERR, ':utf8' );
my $label='swff';
my ($swlexfn,$outfile);
my $split = 0;
GetOptions( 'help|h' => \&usage
	  , 'verbose|v+' => \$main::verbose
	  , 'split' => \$split
	  , 'label=s' => \$label
	  , 'outfile|output|o=s' => \$outfile
	  , 'swlex|s=s' => \$swlexfn
          )
    || die( "FATAL: Option processing failed due to an illegal option\n",
            join(' ',$0,@ARGV), "\n" );
my $rules = shift || '-';

# say good-bye however we exit
$main::start = time();
$SIG{INT} = $SIG{TERM} = sub { exit(42) };

#
# slurp lexica
#
footprint; 
my %sw = ();
debug "reading sw lexicon from $swlexfn";
load_bilex( %sw, $swlexfn );

# open input rule file
if ( substr($rules,-3) eq '.gz' ) {
    die "FATAL: No such file $rules\n" unless -f $rules;
    open( R, "gzip -cd $rules|" ) || die "FATAL: gzip failed: $!\n";
} else {
    open( R, "<$rules" ) || die "FATAL: open $rules: $!\n";
}
binmode( R, ':utf8' );

# alternative output file, open now
if ( defined $outfile ) {
    if ( substr($outfile,-3) eq '.gz' ) {
	open( OUT, "|gzip -c5 > $outfile" ) || die "FATAL: gzip failed: $!\n";
    } else {
	open( OUT, ">$outfile" ) || die "FATAL: open $outfile: $!\n";
    }
    binmode( OUT, ':utf8' );
    select OUT;			# print now goes to OUT, not STDOUT
} else {
    binmode( STDOUT, ':utf8' );
}

footprint;
debug "augmenting rule features with sw ff";
my ($original,@lhsWords,@rhsWords,%features); 
while ( <R> ) {
    debug "read @{[commas($.)]} lines" unless ( $. & 0xFFFFF );
    if ( extract_from_rule( $_, \@lhsWords, \@rhsWords, \%features ) ) {
	# 
	# compute spurious word FF
	#
	my @words = $split ? ( map { permute($_) } @rhsWords ) : @rhsWords;
	my $spurious = 0;
	foreach my $e ( @lhsWords ) {
	    my $seen = 0;
	    foreach my $f ( @words, NULL ) {
		$seen++ if exists $sw{$e}{$f};
	    }
	    $spurious++ unless $seen;
	}

	print $features{id}, "\t$label=$spurious\n";
    } else {
	print STDERR "Info: Ignoring line $.: $_";
    }
}
debug "@{[commas($.)]} lines" if ( $. & 0xFFFFF );

if ( defined $outfile ) {
    close OUT || warn "Warning: close $outfile: $! ($?)\n";
    select STDOUT;		# restore
}

close R || warn "ERROR: close $rules: $!\n";

footprint;
pageinfo ;
debug( "done, ", time()-$main::start, " s duration." )
