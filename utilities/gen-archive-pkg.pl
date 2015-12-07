#!/usr/bin/env perl
#
use 5.008;
use strict;
use File::Basename;
use File::Spec;
use Cwd;
use Getopt::Long qw(:config no_ignore_case);
use File::Path;
use File::Temp qw(tempfile tempdir);
use POSIX ();
use subs qw(log);		# replace math's log with logging

use lib dirname($0);		# where ApplicableRuleFilter resides
use ApplicableRuleFilter;

sub usage {
    print STDERR << "EOF";

--help       print this message
--debug      turn on debugging
--archive    output basename, may include dir, .tar.gz will be appended
--grammar    input grammar (for chunk, after model-1)
--corpus     input sentences (chunk)
--use-brf    generate binarized rule (brf) instead of grammar archive (gar)
--parallel N Attempt to parallelize by degree N (auto-detect on Linux)
             Use N of 1 to force safe single-process sequential processing.
--bin-path   location of itg_binarizer, unknown_word_rules, and archive_grammar
             (if applicable, see --use-brf), if these are different than the
             location of this script. Use before following options

--archive-grammar    explicit location of archive_grammar
--itg-binarizer      explicit location of itg_binarizer
--unknown-word-rules explicit location of unknown_word_rules
--gtar               explicit location of a GNU tar capable of gzip

EOF
    exit(1);
}

my $DEBUG=0;

sub set_debug { 
    $DEBUG=1; 
    arf_set_debug(1); 
}

sub debug_print {
    print STDERR @_ if $DEBUG;
}

sub log {
    my $prefix = POSIX::strftime( "%Y%m%d %H%M%S", localtime );
    # syswrite is more atomic (no such warrantees on NFS)
    syswrite( STDERR, "$prefix @{[join('',@_)]}\n" );
}

sub set_from_path($) {
    my $p = shift;
    ( File::Spec->catfile( $p, 'itg_binarizer' ),
      File::Spec->catfile( $p, 'archive_grammar' ),
      File::Spec->catfile( $p, 'unknown_word_rules' ) );
}

sub detect_parallel() {
    my $result = 1;
    if ( lc($^O) eq 'linux' ) {
	local(*PROC);
	if ( open( PROC, "/proc/cpuinfo" ) ) {
	    $result = 0;
	    while ( <PROC> ) {
		++$result if /^processor\s+:/i;
	    }
	    close PROC;
	}
    }
    $result;
}

my $start = time();
END { 
    warn "# filter ran for ", $main::phase1-$start, " seconds\n"
	if defined $main::phase1;
    warn "# ", basename($0), " ran for ", time()-$start, " seconds\n"; 
}

my $verbose = 0;
my $grammar;
my $corpus;
my $archive_path="./sbmt-pkg";
my $bin_path=dirname($0);
my ($itg_binarizer,$archive_grammar,$uword_rules) =
    set_from_path($bin_path);
my $gtar = '/bin/tar';          # many systems
my $brf = 0;			# create gar files, not brf
my $parallel = detect_parallel();
GetOptions( "help|h"     => \&usage,
	    "archive:s"  => \$archive_path,
	    'use-brf!'   => \$brf,
	    "grammar=s"  => \$grammar,
	    "corpus=s"   => \$corpus,
	    "debug|d"    => \&set_debug,
	    'verbose|v+' => \$verbose,
	    'parallel=i' => \$parallel,
            'itg-binarizer=s' => \$itg_binarizer,
            'archive-grammar=s' => \$archive_grammar,
            'unknown-word-rules=s' => \$uword_rules,
            'gtar=s' => \$gtar,
	    "bin-path:s" => sub {
		if ( defined $_[1] && $_[1] ) {
		    $bin_path = $_[1];
		    ($itg_binarizer,$archive_grammar,$uword_rules) =
			set_from_path($bin_path);
		}
            } )
    || die( "FATAL: Option processing failed due to an illegal option\n",
	    join(' ',$0,@ARGV), "\n" );

log "parallel=$parallel";

# sanity checks
die "FATAL: binary path is not a directory\n" 
    unless -d $bin_path;
die "FATAL: Unable to execute $itg_binarizer\n" 
    unless -x $itg_binarizer;
die "FATAL: Unable to execute $archive_grammar\n" 
    unless ( $brf || -x $archive_grammar );
die "FATAL: Unable to execute $uword_rules\n" 
    unless -x $uword_rules;
die "FATAL: Unable to execute $gtar\n" 
    unless -x $gtar;
die "ERROR: Corpus $corpus does not exist\n" 
    unless -r $corpus;
die "ERROR: Grammar $grammar does not exist\n" 
    unless -r $grammar;

my ($archive_name,$archive_dir) = fileparse($archive_path);
debug_print ("archive-package will be $archive_dir/$archive_path.tar.gz\n");
my $wd = cwd();

mkpath $archive_path 
    or ( rmtree $archive_path and mkpath $archive_path ) 
    or die "FATAL: Couldn\'t create archive-package dir $archive_path";

open( CP, "<$corpus" ) || die "FATAL: Couldn\'t open corpus file $corpus";
my @sentences = <CP>;
close CP;
my $lines = @sentences;
# post-condition: @sentences will have line separators still attached

# Yikes! Sometimes, we just have 1 sentence
$parallel = $lines if ( $parallel > $lines );
log "parallel=$parallel";

# the parallel rule filter can only open as many FDs as the OS permits,
# so we better check the limits here as assertion. The problem is to
# correctly guesstimate the number of FDs required for the |a|b|c pipe.
my $maxfd = POSIX::sysconf( &POSIX::_SC_OPEN_MAX );
die "FATAL: Your chunk\'s size exceeds the number of openable files\n"
    if ( defined $maxfd && 	# sysconf did not fail (undef)
	 $maxfd > 0 && 		# limit is not unlimited (-1 or 0)
	 ($maxfd-10) < @sentences );

# we can only spawn so many processes, and more restrictive environments
# may not like what we attempt to do.
my $maxproc = POSIX::sysconf( &POSIX::_SC_CHILD_MAX );
die "FATAL: Your chunk\'s size exceeds the number of spawnable processes\n"
    if ( defined $maxproc && 	# sysconf did not fail (undef)
	 $maxproc > 0 &&	# limit is not unlimited (-1 or 0)
	 ($maxproc-10) / (4-$brf) < @sentences );

# determine directory for temporary files
my $tmp = $ENV{TMP} || $ENV{TEMP} || $ENV{TMPDIR} || 
    File::Spec->tmpdir() || '/tmp';
my $dir = tempdir( 'gen-XXXXX', CLEANUP => 1, DIR => $tmp );

%main::unlink = ();
END {
    my @fn = keys %main::unlink;
    unlink(@fn) if @fn;
}
$SIG{INT} = $SIG{TERM} = sub { exit(42) };

my (@xrsfile,@xrsname,$sentfn);
for ( my $id=1; $id <= $lines; ++$id ) {
    log "creating id=$id" if $verbose > 1;

    # for each sentence, place it into a file of its own
    $sentfn = File::Spec->catfile( $archive_path, "$id.sent" );
    local(*S);
    open( S, ">$sentfn" ) || die "FATAL: open $sentfn: $!\n";
    print S $sentences[$id-1];
    close S;
    $main::unlink{$sentfn} = 1;

    # open temporary file to write to. 
    ($xrsfile[$id],$xrsname[$id]) = 
	tempfile( "gen-$id-XXXXXX", SUFFIX => '.xrs', 
		  UNLINK => 0, DIR => $dir );
    die "ERROR: open tmp file: $!\n" unless defined $xrsfile[$id];
    $main::unlink{$xrsname[$id]} = 1;
}
log "created $lines temporary files" if $verbose;

my $arf = arf_create(@sentences);

if ( substr($grammar,-3) eq '.gz' ) {
    # popen does not fail, if the argument to gunzip is missing
    die "FATAL: Couldn\'t open grammar $grammar"
        unless -r $grammar;

    # FIXME: This is an implicit dependency on PATH and that gzip
    # actually exists, is findable, and executable. It may break in
    # certain environments.
    open(GRAMMAR, "gzip -cd $grammar|") || die;
} else {
    open(GRAMMAR, "<$grammar") ||
        die "FATAL: Couldn\'t open grammar $grammar";
}

my $mask = 0x1FFFF;
while ( <GRAMMAR> ) {
    my $rule = $_;
    my @rule_matches = arf_all_matches($arf,$rule);
    my $sentid = 1;
    debug_print "@rule_matches \n";
    foreach my $match (@rule_matches) {
        eval {
            if ($match) {
                debug_print "matches sentence $sentid";
                print { $xrsfile[$sentid] } $rule;
            }
        }; 
	die $@ . "sentid:$sentid and rule $rule" if $@;
        $sentid++;
    }
    log( "filtered $. rules" )
	if ( $verbose && ( $. & $mask ) == 0 );
}
log( "filtered $. rules" ) if ( $verbose && ( $. & $mask ) );
close GRAMMAR || die "close $grammar: $!\n"; # "gzip -cd |" may fail here
$main::phase1 = time();

sub construct_cmd($) {
    # globals: @xrsname
    #          $itg_binarizer, $uword_rules, $archive_grammar
    #          $archive_path
    #          $brf
    #
    my $id = shift;

    # itg_binarizer < xrs | unknown_word_rules | archive_grammar
    my $cmd = "$itg_binarizer < $xrsname[$id] " .
        "| $uword_rules -r - -f $archive_path/$id.sent -c true ";
    if ( $brf ) {
        # generate brf rules only
        $cmd .= " -o $archive_path/$id.brf";
    } else {
        # generate gar file
        $cmd .= " -o - | $archive_grammar -o $archive_path/$id.gar";
    }

    $cmd .= " 2>/dev/null";
}

if ( $parallel > 1 ) {
    # parallel execution

    # create who's doing what 
    my ($cpu,@task,@child) = (0);
    for ( my $id=1; $id<=$lines; ++$id ) {
	push( @{$task[$cpu]}, $id );
	$cpu = ($cpu + 1) % $parallel;
	close $xrsfile[$id];
    }

    # branch off task managers
    for ( my $child=0; $child < $parallel; ++$child ) {
	my $pid = fork();
	die "ERROR: fork: $!\n" unless defined $pid;
	if ( $pid > 0 ) {
	    # parent
	    push( @child, $pid );
	} else {
	    # child
	    %main::unlink = ();	# yikes!
	    log "child $child [$$] has ", @{$task[$child]}+0, " tasks";

	    foreach my $id ( @{$task[$child]} ) {
		log "processing file $xrsname[$id]" if $verbose > 1;
		my $cmd = construct_cmd($id);

		if ( system($cmd) != 0 ) {
		    # FIXME: Apparantly, we are missing some pipe errors
		    die "ERROR: ", $?>>8, '/', ($? & 127), " while processing\n";
		}
	    }
	    # avoid any of Perl's clean-up hooks!
	    exec { '/bin/true' } '/bin/true';
	    # oh-well, didn't work (may be a problem on Darwin)
	    exit(0);
	}
    }

    # collect task managers
    for ( my $child=0; $child < $parallel; ++$child ) {
	for (;;) {
	    if ( waitpid( $child[$child], 0 ) > 0 ) {
		die "ERROR: ", $?>>8, '/', ($? & 127), " from $child[$child]\n"
		    unless $? == 0;
		last;
	    }
	}
    }
} else {
    # strictly serial
    for ( my $id=1; $id<=$lines; ++$id ) {
	close $xrsfile[$id];
	log "processing file $xrsname[$id]" if $verbose > 1;
	my $cmd = construct_cmd($id);

	if ( system($cmd) != 0 ) {
	    die "ERROR: ", $?>>8, '/', ($? & 127), " while processing\n";
	}
    }
}

# writing out the instruction-file to be passed to the decoder.
# looks something like:
#
# load-grammar archive "1.gar";
# decode <foreign-sentence> A B C
#
# load-grammar brf "2.brf";
# decode <foreign-sentence> A B C
#
my $instructionfn = File::Spec->catfile( $archive_path, 'decode.ins' );
open( INS, ">$instructionfn" ) || die "FATAL: open $instructionfn: $!\n";
foreach my $id (1 .. scalar(@sentences)) {
    if ( $brf ) {
	print INS "load-grammar brf \"$id.brf\";\n";
    } else {
	print INS "load-grammar archive \"$id.gar\";\n";
    }
    print INS "decode ", $sentences[$id-1], " \n";
}
close INS;


for ( my $id=1; $id<=$lines; ++$id ) {
    my $sentfn = File::Spec->catfile( $archive_path, "$id.sent" );
    delete $main::unlink{$sentfn} if unlink $sentfn;

    # more sanity checks against OOM killer
    my $fn = $brf ? "$archive_path/$id.brf" : "$archive_path/$id.gar";
    warn "Warning! Archive $fn is suspiciously small\n"
	if ( -s $fn < 1000 );
}

my $options = ($DEBUG || $verbose>1) ? '-cvzf' : '-czf';
chdir($archive_dir);
unlink "$archive_name.tar.gz" if -e "$archive_name.tar.gz";

# avoid /bin/sh -c /bin/tar invocation
log( "creating $archive_name.tar.gz in $archive_dir" ) if $verbose;
system { $gtar } ( $gtar, $options, "$archive_name.tar.gz", $archive_name );
chdir($wd);

log( "removing $archive_path" ) if $verbose;
rmtree $archive_path;
exit 0;
