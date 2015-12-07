#!/usr/bin/env perl
#
# Author: graehl

use strict;
use warnings;
use Getopt::Long;

### script info ##################################################
use File::Basename;
my $scriptdir; # location of script
my $scriptname; # filename of script

BEGIN {
   $scriptdir = &File::Basename::dirname($0);
   ($scriptname) = &File::Basename::fileparse($0);
    unshift @INC, $scriptdir;
}

require "libgraehl.pl";

### arguments ####################################################

my $verbose=$ENV{DEBUG};

my $corpus_map="";
my $chunk_sents;
my $longest=999999999999999;
my $max_sents=999999999999999;
my $pad=2;
my $force;
my $outsl;
my $tardir;
my $per;
my @options=(
q{Filter a corpus-map file, or take a list of chunk:sent to produce an instruction file for archives.CHUNK/sent.gar; outputs ins to be placed in current dir},
 ["corpus-map=s"=>\$corpus_map,"Jens' corpus.map as input"],
 ["longest-allowed=s"=>\$longest,"from corpus_map, select only sentences that are at most N foreign words"],
 ["max-sents=s"=>\$max_sents,"keep the first N sentences at most"],
 ["pad-corpus-digits=s"=>\$pad,"corpus numbers are padded to N digits e.g. N=2 archives.01"],
 ["out-map=s"=>\$outsl,"write filtered output chunk:sent list here"],
 ["per-sent-ins=s"=>\$per,"write <per-sent-ins>N.ins instruction file for N=1...#sent"],
 ["map=s"=>\$chunk_sents,"list of chunk:sent instead of corpus-map"],
 ["force-strings=s"=>\$force,"OPTIONAL (makes force_decoder ins output) expected (forced) decodings in corpus order"],
 ["tar-dir=s"=>\$tardir,"path for archive dirs 02/archives.tar.gz"]
);

&show_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
show_opts(@opts);

#&argvz;

my @foreign; # $foreign[chunk]->[sent]

sub zeropad {
    my ($n,$p)=@_;
    $p=2 unless defined $p;
    sprintf("%0${p}d",$n);
}

sub apad {
    zeropad($_[0],$pad);
}


sub archive_dir {
    my ($i)=@_;
    "archives.".&apad($i);
}

sub archive_tar {
    my ($i)=@_;
    my $p=&apad($i);
    die "can't find tar for archive $p because -tar-dir isn't specified" unless $tardir;
    "$tardir/$p/archives.tar.gz";
}

# assumes "load-grammar archives.02/3.gar;"
sub get_foreign {
    my ($c,$s)=@_;
    return $foreign[$c]->[$s] if defined $foreign[$c];
    my $f=$foreign[$c]=[undef]; # $s is 1-indexed.
    my $ins=&archive_dir($c)."/decode.ins";
    if (! -f $ins) {
        my $a=&archive_tar($c);
        my $cmd="tar xzf $a 1>&2";
        info("unpacking tar for $c:$s: $cmd");
        system $cmd;
        die "$cmd: $?" if $?>>8;
    }
    open INS,"<",$ins or die "can't read $ins";
    while (<INS>) {
        &debug($c,$_) if (/^load-grammar/);
        if (/^decode (.*)$/) {
            push @$f,$1;
        }
    }
    close INS;
    $f->[$s];
}

my @idtext;

if ($corpus_map) {
    @idtext=`grep -v ^# $corpus_map`;
    @idtext = grep { my @f=split; &debug($3,$_); $f[3] <= $longest } @idtext;
    @idtext = exec_filter("sort -nk 3",@idtext);
}

if ($chunk_sents) {
    read_file_lines_ref($chunk_sents,\@idtext);
}

die "no sentences found" unless scalar @idtext;

splice @idtext,$max_sents if $max_sents<scalar @idtext;

if ($outsl) {
    my $O=openz_out($outsl);
    print $O @idtext;
}

my @forced;
read_file_lines_ref($force,\@forced) if $force;

my $i=-1;
mkdir_check(dir_file($per)) if $per;
my $fh=\*STDOUT;
for (@idtext) {
    next if /^\s+$/;
    ++$i;
    die "expected lines containing chunk:sentid but got $_" unless /\b(\d+)\:(\d+)\b/;
    my ($c,$s)=($1,$2);
    &debug($c,$s);
    my $f=get_foreign($c,$s);
    die "chunk $c sent $s foreign sentence not found" unless $f;
    if ($per) {
        $fh=openz_out("$per$i.ins");
    }
    print $fh 'load-grammar archive "'.archive_dir($c).qq{/$s.gar";\n};
    if ($force) {
        die "no force-string for corpus-ordered sentence #$i chunk $c sent $s" unless $forced[$i];
        my $tar=$forced[$i];
        chomp $tar;
        print $fh <<EOF;
force-decode {
 source: $f
 target: $tar
}

EOF
    } else {
        print $fh "decode $f\n\n";
    }
    close $fh if $per;
}
