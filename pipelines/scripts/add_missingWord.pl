#!/usr/bin/env perl
#
# Adds missingWord feature. This perl works on raw bytes. 
# 2008-05-05 JSV
#
use 5.006;
use strict;
use File::Spec;
use File::Basename qw(basename dirname);
use Getopt::Long qw(:config bundling no_ignore_case);
use Carp;

use lib dirname($0);
use NLPTools qw(:all);
use NLPRules qw(:all);

# The bilexica do no use <unk>
use constant NULL => 'UNKNOWN_WORD';

# global
$main::verbose = 0;

sub usage {
    my $prg = basename($0,'.pl');
    print << "EOF";
Usage: $prg [options] --mwlex lexicon[.gz] [ rules[.gz] ]

Optional arguments:
 -v|--verbose             Increase verbosity level.
 -o|--output fn[.gz]      Put output into fn instead of STDOUT.

Mandatory arguments:
 -m|--mwlex fn[.gz|.bz2]  missing word bilexicon.
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

    my $mask = 0xFFFFF;
    my $n = 0;
    my (@x,$y1,$y2);
    while ( <T> ) {
	chomp ;
	footprint(commas($.)) if ( $main::verbose && ( $. & $mask ) == 0 );
	# swFF:     count   trg   src
	# mwFF:     count   src   trg
	if ( (@x = split()) == 3 ) {
	    $tref->{$x[1]}{$x[2]} = $x[0];
	    $n++;
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

#
# --- main ------------------------------------------------------
#
debug( "$0 @ARGV" );
my $label='mwff';
my ($mwlexfn,$outfile);
GetOptions( 'help|h' => \&usage
	  , 'verbose|v+' => \$main::verbose
	  , 'outfile|output|o=s' => \$outfile
	  , 'mwlex|m=s' => \$mwlexfn
	  , 'label=s' => \$label
          )
    || die( "FATAL: Option processing failed due to an illegal option\n",
            join(' ',$0,@ARGV), "\n" );
my $rules = shift || '-';

# say good-bye however we exit
$main::start = time();
$SIG{INT} = $SIG{TERM} = sub { exit(42) };

#
# slurp bi-lexicon
#
footprint; 
my %mw = ();
debug "reading mw lexicon from $mwlexfn";
load_bilex( %mw, $mwlexfn );

# open input rule file
if ( substr($rules,-3) eq '.gz' ) {
    die "FATAL: No such file $rules\n" unless -f $rules;
    open( R, "gzip -cd $rules|" ) || die "FATAL: gzip failed: $!\n";
} else {
    open( R, "<$rules" ) || die "FATAL: open $rules: $!\n";
}

# alternative output file, open now
if ( defined $outfile ) {
    if ( substr($outfile,-3) eq '.gz' ) {
	open( OUT, "|gzip -c5 > $outfile" ) || die "FATAL: gzip failed: $!\n";
    } else {
	open( OUT, ">$outfile" ) || die "FATAL: open $outfile: $!\n";
    }
    select OUT;			# print now goes to OUT, not STDOUT
}

footprint;
debug "augmenting rule features with mw ff";
my ($original,@lhsWords,@rhsWords,%features); 
while ( <R> ) {
    debug "@{[commas($.)]} lines" unless ( $. & 0xFFFFF );
    if ( extract_from_rule( $_, \@lhsWords, \@rhsWords, \%features ) ) {
	# 
	# compute missing word FF
	#
	my $missing = 0;
	foreach my $f ( @rhsWords ) {
	    my $seen = 0;
	    foreach my $e ( @lhsWords, NULL ) {
		$seen++ if exists $mw{$f}{$e};
	    }
	    $missing++ unless $seen;
	}

	print $features{id}, "\t$label=$missing\n";
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
