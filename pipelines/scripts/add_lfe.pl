#!/usr/bin/env perl
#
# Given a (raw) "the lexicon" in invers direction, and rules with align
# attribute, compute the lex_inv feature according to the xxxx paper.
#
use 5.006;
use strict; 
use File::Spec;
use File::Basename;
use File::Temp qw(tempfile);
use Getopt::Long qw(:config bundling no_ignore_case);
use Cwd ();

use lib dirname($0);
use NLPTools qw(:all);
use NLPRules qw(:all);

use constant PENALTY => -20;	# worst cost for unknown
use constant NULL => 'NONE'; 

my $verbose = 0; 

# the initialization values come from bilex computation filter settings. 
my $invers_penalty = log(0.01 / 2); 
my $invers_null_penalty = log(0.03 / 2); 
my $label = 'lfe';
sub usage {
    my $prg = basename($0,'.pl');
    print << "EOF";
Usage: $prg [options] [ - | rules[.gz]]

Optional arguments:
 -h|--help             print this help and exit.
 -v|--verbose          increase verbosity level. 
 -o|--output fn[.gz]   produce output into fn instead of stdout.
 --bad                 die if a word pair is not found in lexicon. 
 --rejects fn          put rules into when word pairs not found.

Optional penalty exponents, if lexicon lacks an alignment: 
 --invers-penalty exp     invers penalty e, if P(f|e) is not found: $invers_penalty
 --invers-null-penalty e  invers penalty e, if P(f|NULL) is not found: $invers_null_penalty
 --penalty exp            set all of above penalties to same exp. 

Mandatory arguments:
 -I|--invers fn   "invers" alignment lexicon (c,e,f) file fn for P(f|e)

Mandotory argument: 
 rules[.gz]            input xrs rule file, use - for stdin. 

EOF
    exit 1;
}

sub load_bilex(\%$) {
    # purpose: load a possibly compress t-table into a hash
    # paramtr: %ttable (OUT): new t-table to put things into.
    #          The GIVEN-VAR is the 1st-level hash, the OF-VAR 2nd-level. 
    #          e.g. input file <c,f,e> will create P(e|f) with f on 1st.
    #          $fn (IN): name of the file to load t-table from
    # warning: levels are reversed for missing and spurious word
    # returns: number of valid elements processed.
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
    my (@x,$y1,$y2,%group);
    while ( <T> ) {
	chomp ;
	footprint(commas($.)) if ( $main::verbose && ( $. & $mask ) == 0 );
	# mwFF:     count   src   trg (normal) P(e|f)
	# swFF:     count   trg   src (invers) P(f|e)
	if ( (@x = split()) == 3 ) {

## start: code path 1
#
#	    $tref->{$x[1]}{$x[2]} = $x[0];
#	    $group{$x[1]} += $x[0] if $x[2] ne NULL;
#	    ++$n; 
#
## final: code path 1

## start: code path 2
#
	    if ( $x[0] ne NULL ) {
		$tref->{$x[1]}{$x[0]} = $x[2];
		$group{$x[1]} += $x[2]; 
		++$n; 
	    }
#
## final: code path 2

	} else {
	    warn( "Info: Line ", commas($.), ": Bad line, skipping\n" );
	}
    }
    footprint(commas($.)) if ( $main::verbose && ( $. & $mask ) );
    pageinfo ;
    close T || die "FATAL: close $fn: $!\n";

    # create probabilities from counts using groups
    debug( "converting counts into probs by group" ) if $main::verbose; 
    foreach my $one ( keys %{$tref} ) {
	foreach my $two ( keys %{$tref->{$one}} ) {
	    if ( exists $group{$one} ) {
		$tref->{$one}{$two} /= $group{$one};
	    } else {
		# avoid division by zero
		$tref->{$one}{$two} = 1.0;
	    }
	}
    }

    # done
    debug( commas($n), ' pairs extracted in ', time()-$start, ' s' ) if $n; 
    $n;
}

%main::xlate = ( s => 'source', t => 'target' );

sub parse_align($) {
    # purpose: parses the align feature on a rule
    # paramtr: $align (IN): the align feature's value
    # returns: 
    #
    my $align = substr( shift(), 1, -1 ); # remove brackets

    my %result = ( pairs => 0 ); 
    foreach ( split ' ',$align ) {
	if ( /^(\d+),(\d+)$/ ) {
	    # $1:s $2:t 
	    push( @{$result{align}[$2]}, $1 ); 
	    push( @{$result{revers}[$1]}, $2 );
	    $result{pairs}++; 
	} elsif ( /\#([st])=(\d+)/ ) {
	    $result{ $main::xlate{$1} } = $2; 
	}
    }

    %result; 
}

#
# --- main ------------------------------------------------------
#
my $tmp = $ENV{'MY_TMP'} ||     # Wei likes MY_TMP, so try that first
    $ENV{TMP} ||                # standard
    $ENV{TEMP} ||               # windows standard
    $ENV{TMPDIR} ||             # also somewhat used
    File::Spec->tmpdir() ||     # OK, this gets used if all above fail
    '/tmp';                     # last resort

my $bad = 0; 			# when unfiltered, NOTFOUND is a bug
my $outfile = '-'; 
my ($rejects,%invers,$inversfn); 
GetOptions( 'help|h' => \&usage
	  , 'verbose|v+' => \$verbose
	  , 'outfile|output|o=s' => \$outfile
          , 'tmp=s' => \$tmp
	  , 'bad' => \$bad
	  , 'label=s' => \$label
	  , 'rejects|reject=s' => \$rejects
	  , 'invers|I=s' => \$inversfn
	  , 'invers-penalty=f' => \$invers_penalty
	  , 'invers-null-penalty=f' => \$invers_null_penalty
	  , 'penalty=f' => sub {
	      $invers_penalty = $invers_null_penalty = $_[1] }
          )
    || die( "FATAL: Option processing failed due to an illegal option\n",
            join(' ',$0,@ARGV), "\n" );

my $rules = shift || usage;
usage() unless $inversfn;

if ( substr($rules,-3) eq '.gz' ) {
    die "FATAL: No such file $rules\n" unless -r $rules; 
    open( R, "gzip -cd $rules|" ) || die "FATAL: run gzip: $!\n"; 
} else {
    open( R, "<$rules" ) || die "FATAL: open $rules: $!\n";
}

if ( substr($outfile,-3) eq '.gz' ) {
    open( OUT, "|gzip -c9 > $outfile" ) || die "FATAL: run gzip: $!\n"; 
} else {
    open( OUT, ">$outfile" ) || die "FATAL: open $outfile: $!\n"; 
}

if ( defined $rejects ) {
    if ( substr($rejects,-3) eq '.gz' ) {
	open( REJECTS, "|gzip -c5 > $rejects" ) || die "FATAL: run gzip: $!\n";
    } else {
	open( REJECTS, ">$rejects") || die "FATAL: open $rejects: $!\n"; 
    }
}

# load invers lexicon -- no filtering
load_bilex( %invers, $inversfn ); 

# report chosen penalties
debug "invers penalty $invers_penalty and invers NULL penalty $invers_null_penalty";

my ($p,@xrs);
my $mask = 0xFFFFF; 
my $null = NULL;
RULE: while ( <R> ) {
    footprint(commas($.) . ' rules') unless ( $. & $mask ); 
    if ( (@xrs=extract_lrf_fast($_)) == 3 && index($xrs[1],FOREIGN) == -1 ) {
	my @target = target_wordvar($xrs[0]); 
	my @source = source_token($xrs[1]);
	my %feat = feature_spec($xrs[2]); # need id and align features
	my %x = parse_align( $feat{align} ); 

	if ( @target != $x{target} ) {
	    debug( "target word mis-match: $x{target} vs ", 0+@target, ", skipping" );
	    next; 
	}
	if ( @source != $x{source} ) {
	    debug( "source word mis-match: $x{source} vs ", 0+@source, ", skipping" );
	    next; 
	}

	print STDERR "\n$_\n" if $verbose > 1; 
	if ( $verbose > 2 ) {
	    print STDERR "# source:"; 
	    for ( my $s=0; $s < @source; ++$s ) {
		printf STDERR " %d:%s", $s, $source[$s]; 
	    }
	    print STDERR "\n# target:"; 
	    for ( my $t=0; $t < @target; ++$t ) {
		printf STDERR " %d:%s", $t, $target[$t]; 
	    }
	    print STDERR "\n\n"; 
	}

	#
	# phase: invers P(f|e) 
	#
	my $pfinal = 0.0; 
	my $x = 0; 
	for ( my $s=0; $s < @source; ++$s ) {
	    next unless index($source[$s],'"') == 0;
	    my $sword = substr($source[$s],1,-1);
	    ++$x; 
	    
	    if ( defined $x{revers}[$s] ) {
		my $n = 0; 
		my $psum = 0.0; 
		my $max = @{$x{revers}[$s]};
		foreach my $t ( @{$x{revers}[$s]} ) {
		    next unless index($target[$t],'"') == 0; 
		    my $tword = substr($target[$t],1,-1);
			
		    ++$n; 
		    if ( exists $invers{$tword}{$sword} ) { 
			$p = $invers{$tword}{$sword};
			if ( $verbose > 2 ) {
			    printf STDERR "P%d", $x; 
			    printf STDERR "_%d", $n if $max > 1;
			    printf STDERR "(%s|%s)=%g\n", 
			    $source[$s], $target[$t], $p; 
			}
		    } else {
			# NOT in lexicon -- bad!
			print STDERR "$_\n"
			    if ( (defined $rejects || $bad) && $verbose < 2 ); 
			$p = exp($invers_penalty); 
			if ( $verbose > 2 || $bad || defined $rejects ) {
			    printf STDERR "P%d", $x; 
			    printf STDERR "_%d", $n if $max > 1;
			    printf STDERR "(%s|%s)=NOT_FOUND [%g]\n", 
			    $source[$s], $target[$t], $p; 
			}
			die "at $rules:$.\n" if $bad; 
			if ( defined $rejects ) {
			    print REJECTS $_; 
			    next RULE;
			}
		    }
		    $psum += $p; 
		}
		if ( $n ) {
		    $psum /= $n; 
		    printf STDERR "P%d(...)=%g\n", $x, $psum
			if ( $n > 1 && $verbose > 2 ); 
		    $pfinal += ( $psum ? log($psum) : PENALTY ); 
		}
	    } else {
		# unaligned
		if ( exists $invers{$null}{$sword} ) {
		    $p = $invers{$null}{$sword};
		    if ( $verbose > 2 ) {
			printf STDERR "P%d(%s|NULL)=%g\n", 
			$x, $source[$s], $p;
		    }
		    $pfinal += $p ? log($p) : PENALTY;
		} else {
		    # NOT in lexicon -- bad!
		    print STDERR "$_\n"
			if ( (defined $rejects || $bad) && $verbose < 2 ); 
		    $p = exp($invers_null_penalty); 
		    if ( $verbose > 2 || $bad || defined $rejects ) {
			printf STDERR "P%d(%s|NULL)=NOT_FOUND [%g]\n", 
			$x, $source[$s], $p;
		    }
		    die "at $rules:$.\n" if $bad; 
		    if ( defined $rejects ) {
			print REJECTS $_; 
			next RULE;
		    }
		    $pfinal += $invers_null_penalty; 
		}
	    }
	}
	if ( $verbose > 1 ) {
	    printf STDERR "P(f|e)=e^%.9f=%g\n", $pfinal, exp($pfinal); 
	    printf STDERR "\n" if $verbose > 2; 
	}

	printf OUT  "%s\t%s=e^%.6f\n", $feat{id}, $label, $pfinal; 
    }
}
footprint(commas($.) . ' rules') if ( $. & $mask ); 

close R || warn "Warning: close $rules: $!\n"; 
close OUT || warn "Warning: close $outfile: $!\n"; 
if ( defined $rejects ) {
    close REJECTS || warn "Warning: close $rejects: $!\n"; 
}
pageinfo; 
