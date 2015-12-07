#!/usr/bin/env perl 
#
# $0 < xrs_rules > output_rules
# $0 xrs_rules > output_rules
# $0 xrs1 xrs2 > output_rules
#
# Add head information to xrs rules, and add the headmarker={{{...}}} to
# the rules.
#
use 5.006;
use strict;
use warnings;

use File::Basename;
use File::Spec;
use File::Copy;			# for debugging
use Cwd qw(chdir abs_path);
use File::Temp qw(tempfile tempdir);
use Fcntl qw(:seek);
use Getopt::Long qw(:config bundling no_ignore_case);

# !!! CHANGE ME !!!
#my $bindir = q{/home/nlg-03/wang11/sbmt-bin/v3.0/dlm/v1.0};
my $bindir = dirname($0);

sub usage {
    my $app = basename($0);
    print << "EOF";
Usage: $app [options] xrs [xrs [..]] > out
Usage: $app -o out[.gz] [options] xrs [xrs [..]]

Optional arguments:
 -h|--help       print this help and exit.
 -o|--output fn  puts output into fn instead of stdout.
 --tmp dir       where to put temporary files.
 --collins fn    location of the collins.rules extra file, see --treep.
 --keep dir      debugging: where to copy intermediary tmp files to.

Mandatory arguments:
 --treep fn          location of the treep program. Setting treep will
                     update --collins, so use --collins after --treep.
 --mark-head-xrs fn  location of the mark_head_xrs program (decoder).

EOF
    exit 1;
}

#
# --- main ------------------------------------------------------
#

usage unless @ARGV;
my @SAVE = ( @ARGV );

my $tmp = $ENV{'MY_TMP'} ||     # Wei likes MY_TMP, so try that first
    $ENV{TMP} ||                # standard
    $ENV{TEMP} ||               # windows standard
    $ENV{TMPDIR} ||             # also somewhat used
    File::Spec->tmpdir() ||     # OK, this gets used if all above fail
    ( -d '/scratch' ? '/scratch' : '/tmp' ); # last resort

my $treep = File::Spec->catfile( $bindir, 'treep' );
my $collins = File::Spec->catfile( dirname($treep), 'collins.rules' );
my ($mark_head_xrs,$outfile,$keep);
GetOptions( 'help|h' => \&usage
	  , 'outfile|output|o=s' => \$outfile
#	  , 'treep=s' => sub {
#	      $treep = $_[1];
#	      $collins = File::Spec->catfile( dirname($treep), 'collins.rules' );
#	  }
	  , 'treep=s' => \$treep
	  , 'collins=s' => \$collins
	  , 'mark-head-xrs=s' => \$mark_head_xrs
          , 'tmp=s' => \$tmp
	  , 'keep=s' => \$keep
          )
    || die( "FATAL: Option processing failed due to an illegal option\n",
            join(' ',$0,@SAVE), "\n" );

# hello
warn "# $0 @SAVE\n";

# sanity checks
die "FATAL: Unable to execute $treep\n" unless -x $treep;
die "FATAL: Unable to read $collins\n" unless -r $collins; 
die "FATAL: Unable to execute $mark_head_xrs\n" unless -x $mark_head_xrs;

# separate the input rules into two files. one file only the LHS the
# other file the rest. LHS's in the first file has converted into format
# that is friendly to treep.
#
my ($fh0, $file0) = tempfile( 'add-dlmstr-xrs-XXXXXX', CLEANUP => 1, DIR => $tmp );
die "ERROR: create file in $tmp: $!\n" unless defined $fh0;
warn "# file0=$file0\n";

if ( defined $outfile ) {
    if ( substr($outfile,-3) eq '.gz' ) {
	open( OUT, "| gzip -c5 > $outfile" ) ||
	    die "FATAL: gzip failed: $!\n";
    } else {
	open( OUT, ">$outfile" ) ||
	    die "FATAL: open $outfile: $!\n";
    }
    select OUT;			# redirect stdout
}

# 
# phase 1: copy input or stdin to temp file0
#
my $rules = 0;
while ( <> ) {
    if ( /^%%%/ ) {
	print ;
    } else {
	++$rules;
	print $fh0 $_;
    }
}
warn "# $. lines in input file(s), $rules rules\n";
if ( $rules == 0 ) {
    warn "# no input rules, jumping to exit\n";
    goto FAST_EXIT;
}

#
# phase 2: process temp file0 into file1 and file2
#
my ($fh1, $file1) = tempfile( 'add-dlmstr-xrs-XXXXXX', CLEANUP => 1, DIR => $tmp );
die "ERROR: create file in $tmp: $!\n" unless defined $fh1;
warn "# file1=$file1\n";

my ($fh2, $file2) = tempfile( 'add-dlmstr-xrs-XXXXXX', CLEANUP => 1, DIR => $tmp );
die "ERROR: create file in $tmp: $!\n" unless defined $fh2;
warn "# file2=$file2\n";

seek( $fh0, 0, SEEK_SET ) || die "FATAL: seek $file0: $!\n";
copy( $file0, "$keep/file0" ) if defined $keep;
my $count = 0;
while( <$fh0> ){
    s/[\015\012]+$//;
    if ( /^\s*(.*) (-> .* \#\#\# .*)$/ ) {
        my $lhs = $1;
        print $fh2 "$2\n";

	#$lhs = "(" . $lhs . ")";
        $lhs =~ s/\)\("\)"\)/-RRB-\("-RRB-"\)/g;
        $lhs =~ s/-RRB-\("\)"\)/-RRB-\("-RRB-"\)/g;
        $lhs =~ s/\(\("\("\)/-LRB-\("-LRB-"\)/g;
        $lhs =~ s/-LRB-\("\("\)/-LRB-\("-LRB-"\)/g;
        #$lhs =~ s/([^\(]+)\("(.*?)"\)/$1\($2\)/g;
        $lhs =~ s/x\d+://g;
        $lhs =~ s/([^ \(]+)\(/\($1 /g;
        print $fh1 "$lhs\n";
	$count++
    }
}
warn "# $. lines from $file0\n";

#
# phase 3: do something with file1 to create file3
#
my ($fh3, $file3) = tempfile( 'add-dlmstr-xrs-XXXXXX', CLEANUP => 1, DIR => $tmp );
die "FATAL: create file in $tmp: $!\n" unless defined $fh3; 
warn "# file3=$file3\n";

# just in case
seek( $fh1, 0, SEEK_SET ) || die "FATAL: seek $file1: $!\n"; 
copy( $file1, "$keep/file1" ) if defined $keep;

#
# [1.1] read stdin by redirecting stdin, NOT by cat
# [1.2] propagating errors from starting the 2nd+ program does not work,
#       i.e. the popen will never properly fail!!!
# [2] pass stderr through to this process's stderr to handle
# [3.1] we already checked that we can execute $treep and $collins
# [3.2] check that the input exists, or it will be a deferred error
#
my $cmd = "$treep $collins < $file1 |";
warn "# $cmd\n";
open( PIPE, $cmd ) || die "FATAL: popen $treep: $! ($?)\n";
while ( <PIPE> ) {
    s/[\015\012]+$//;
    s/\(([^\( ]+)/$1\(/g;
    # PP("twice"-H)
    s/"-H\)/"\)/g;
    my @a = split /\s+/;
    my $index = 0;
    my @b;
    foreach my $w (@a) {
        if($w =~ /\($/  || $w =~ /".*"/){
            push @b, $w;
        } else {
            push @b, "x$index:$w";
            $index++;
        }
    }

    my $l = join(' ', @b);
    #$l =~ s/ //g;
    #$l =~ s/\(([^\)\( ]+)\)/\("$1"\)/g;
    $l =~ s/\( /\(/g;
    print $fh3 "$l\n";
}
warn "# $. lines from pipe\n";

# pclose may fail NOW with deferred errors (e.g. input file not found)
# you MUST check every pclose to any popen call
close PIPE || die "pclose $treep: $! ($?)\n";
close $fh1; 			# not needed after this point

#
# phase 4: do something with file2 and file3 to create file4
#
seek( $fh2, 0, SEEK_SET ) || die "FATAL: seek $file2: $!\n"; 
copy( $file2, "$keep/file2" ) if defined $keep;
seek( $fh3, 0, SEEK_SET ) || die "FATAL: seek $file3: $!\n"; 
copy( $file3, "$keep/file3" ) if defined $keep;

my ($fh4, $file4) = tempfile( 'add-dlmstr-xrs-XXXXXX', CLEANUP => 1, DIR => $tmp );
die "FATAL: create file in $tmp: $!\n" unless defined $fh4;
warn "# file4=$file4\n";

#
# [1] DO NOT use "export x=y" in system, because plain Bourne shells
#     will fail on it. "export x=y" is a Linux/bash unportable spec!
$cmd  = "paste -d' ' $file3 $file2 |";
$cmd .= " env LD_LIBRARY_PATH=$bindir";
$cmd .= ':' . $ENV{LD_LIBRARY_PATH} if exists $ENV{'LD_LIBRARY_PATH'}; 
$cmd .= " $mark_head_xrs $collins > $file4";
warn "# $cmd\n"; 
system($cmd) == 0 ||
    die "FATAL: paste or $mark_head_xrs failed with $?\n";

close $fh2;			# not needed after this point
close $fh3;			# not needed after this point
copy( $file4, "$keep/file4" ) if defined $keep;

#
# phase 5: munge file0 and file4 into output on stdout
#
seek( $fh0, 0, SEEK_SET ) || die "FATAL: seek $file0: $!\n";
seek( $fh4, 0, SEEK_SET ) || die "FATAL: seek $file4: $!\n";

$count = 0;
for(;;) {
    my $l1 = <$fh0>;
    my $l2 = <$fh4>;
    ++$count; 

    if ( defined($l1) && defined($l2) ) {
        $l1 =~ s/[\015\012]+$//;
        $l2 =~ s/[\015\012]+$//;
        $l1 =~ s/\)\("\)"\)/-RRB-\("\)"\)/g;
        $l1 =~ s/\(\("\("\)/-LRB-\("\("\)/g;
        if ( $l2 =~ /(headmarker={{{.*?}}})/ ) {
            print "$l1 $1\n";
        } else {
	    print "$l1\n";
	}
    } else  {
	die "MIS-ALIGN! Possiblly due to comments!\n"
	    if ( defined($l1) && !defined($l2)  || !defined($l1) && defined($l2) );
        last;
    }
}
close $fh0;			# not needed after this point
close $fh4;			# not needed after this point
warn "# $count lines in output file\n"; 

FAST_EXIT:
if ( defined $outfile ) {
    select STDOUT;		# restore stdout from redirection
    close OUT || warn "Warning: close $outfile: $! ($?)\n";
}

# this may fail, and we don't care
unlink( map { defined $_ } ( $file0, $file1, $file2, $file3, $file4 ) );

exit 0;
