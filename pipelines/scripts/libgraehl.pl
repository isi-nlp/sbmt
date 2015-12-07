###USAGE: begin your perl script with:

# use warnings;
# use strict;
# use Getopt::Long;
# use File::Basename;
# my $scriptdir; # location of script
# my $scriptname; # filename of script
# my $BLOBS;
# BEGIN {
#    $scriptdir = &File::Basename::dirname($0);
#    ($scriptname) = &File::Basename::fileparse($0);
#    push @INC, $scriptdir;
#     $ENV{BLOBS}='/home/hpc-22/dmarcu/nlg/blobs' unless exists $ENV{BLOBS};
#     $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
#     $BLOBS=$ENV{BLOBS};
#     my $libgraehl="$BLOBS/libgraehl/v4";
#     push @INC,$libgraehl if -d $libgraehl;
# }
# require "libgraehl.pl";


# $fh=openz($filename)
# &argvz; <>;
# exec_filter('cat',"a\nb") eq "a\nb"
# &cleanup would kill the current exec_filter process - naturally this assumes only one runs at a time per process (the global filehandles FW/FR require this as well)

use warnings;
use strict;

sub flush {
    my ($fh)=@_;
    my($old) = select($fh) if defined $fh;
    $| = 1;
    print "";
    $| = 0;
    select($old) if defined $fh;
}

sub unbuffer {
    my ($fh)=@_;
    my($old) = select($fh) if defined $fh;
    $| = 1;
    print "";
    select($old) if defined $fh;
}


use Cwd qw(getcwd abs_path chdir);

sub getcd {
# my $curdir=`pwd`;
# chomp $curdir;
# $curdir =~ s|/$||;
# return $curdir;
  return getcwd;
}

my $DEBUG=$ENV{DEBUG}; 
our $debug_fh=\*STDERR;
sub set_debug_fh {
    my $oldfh=$debug_fh;
    $debug_fh=$_[0] if defined $_[0];
    return $oldfh;
}

require 'dumpvar.pl';

sub debugging {
 return $DEBUG;
}

sub debug {
    if ($DEBUG) {
        no warnings 'uninitialized';
        my ($package, $filename, $line) = caller;
        $filename = $1 if $filename =~ m|/([^/]+)$|;
        my $dbg=((!defined($package) || $package eq 'main') ? '' : "[$package]")."$filename($line): ";
        my $oldfh=select($debug_fh);
        my $first=1;
        for (@_) {
            if ($first) {
                $first=0;
                print $dbg;
            } else {
                print '; ';
            }
            if (ref($_)) {
                print ref($_),': ';
                dumpValue($_);
                $first=1;
            } else {
                print $_;
            }
        }
        print "\n";
        select($oldfh);
    }
}

my $is_shell_special=qr.[ \t\n\\><|&;"'`~*?{}$!()].;
my $shell_escape_in_quote=qr.[\\"\$`!].;

sub nthline {
    my ($line,$file) = @_;
    local (*NTH);
    open NTH,"<",$file or die "Couldn't open $file: $!";
    my $ret;
    while(<NTH>) {
        if (--$line == 0) {
            $ret=$_;
            last;
        }
    }
    close NTH;
    return $ret;
}

sub escape_shell {
    my ($arg)=@_;
    return undef unless defined $arg;
    if ($arg =~ /$is_shell_special/) {
        $arg =~ s/($shell_escape_in_quote)/\\$1/g;
        return "\"$arg\"";
    }
    return $arg;
}

sub cmdline {
    return join ' ',($0,@ARGV);
}

sub escaped_shell_args {
    return map {escape_shell($_)} @_;
}

sub escaped_shell_args_str {
    return join ' ',&escaped_shell_args(@_);
}

sub escaped_cmdline {
    return "$0 ".&escaped_shell_args_str(@ARGV);
}


#returns ($dir,$base) s.t. $dir$base accesses $pathname and $base has no
#directory separators
sub dir_file {
    my ($pathname)=@_;
    if ( $pathname =~ m|/| ) {
        die unless $pathname =~ m|(.*/)(.*)|;
        return ($1,$2);
    } else {
        return ('./',$pathname);
    }
}

#no slash between dir and file.  if absolute, then dir='/'; if relative, dir='.'
sub dir_file_noslash {
    my ($dir,$file)=&dir_file(@_);
    $dir =~ s|/$|| unless $dir eq '/';
    $dir = '.' unless $dir;
    return ($dir,$file);
}

sub mydirname {
    my ($dir,$file)=&dir_file_noslash(@_);
    return $dir;
}

sub mybasename {
    my ($dir,$file)=&dir_file_noslash(@_);
    return $file;
}

sub which_in_path {
    my ($prog)=@_;
    my $which='/usr/bin/which';
    $which='which' unless -x $which;
    $prog=`$which $prog 2>/dev/null`;
    chomp $prog;
    return $prog;
}

our $info_fh=\*STDERR;

sub set_info_fh {
    my $oldfh=$info_fh;
    $info_fh=$_[0] if defined $_[0];
    return $oldfh;
}

sub chomped {
    my ($a)=@_;
    chomp $a;
    return $a;
}

sub info {
#substr($_[$#_],-1) eq "\n" ? "" :
    print $info_fh @_, "\n";
}

my %info_counts=();

sub count_info {
    no warnings;
    ++$info_counts{$_[0]};
}

sub info_remember {
    my ($f)=@_;
    &info(@_);
    count_info($f);
}

sub info_remember_quiet {
    &debug;
    &count_info;
}

sub info_summary {
    return unless scalar %info_counts;
    my ($fh)=@_;
    local($info_fh)=$fh if defined $fh;
    info("(<#occurrences> <information type>):");
    info(sprintf('% 9d',$info_counts{$_})," $_") for (sort keys %info_counts);
    &info_runtimes;
}

sub info_runtimes {
    info("(total running time: ".&runtimes.")\n");
}

sub warning {
    my ($f,@r)=@_;
    info_remember("WARNING: $f",@r);
    return undef;
}

sub which_prog {
    my ($prog,$defaultprog,$quiet,$nodie)=@_;
    $defaultprog='' unless defined $defaultprog;
    $quiet=0 unless defined $quiet;
    $nodie=0 unless defined $nodie;
    my $absprog = which_in_path($prog);
    my $error='';
    unless (-x $absprog) {
        my $scriptdir=&mydirname;
        $absprog="$scriptdir/$prog";
        unless (-x $absprog) {
            if ($defaultprog) {
                $absprog=$defaultprog;
                $error="Error: couldn't find an executable $prog in PATH, in $scriptdir, or at default $defaultprog!" unless -x $absprog;
            } else {
                $error="Error: couldn't find an executable $prog in PATH or in $scriptdir!";
            }
        }
    }
    if (-x $absprog) {
        info("Using $prog at $absprog") unless $quiet;
        return $absprog;
    }
    if ($nodie) {
        info($error);
        return '';
    } else {
        die $error ;
    }
}

sub which {
    my ($fullprog,$defaultprog,$quiet,$nodie)=@_;    
    $fullprog =~ /(\S+)(.*)/ && return &which_prog($1,$defaultprog,$quiet,$nodie) . $2;
}

sub expand_symlink {
    my($old) = @_;
    local(*_);
    my $pwd=`pwd`;
    chop $pwd;
    $old =~ s#^#$pwd/# unless $old =~ m#^/#;  # ensure rooted path
    my @dir = split(/\//, $old);
    shift(@dir);                # discard leading null element
    $_ = '';
  dir: foreach my $dir (@dir) {
        next dir if $dir eq '.';
        if ($dir eq '..') {
            s#/[^/]+$##;
            next dir;
        }
        $_ .= '/' . $dir;
        while (my $r = readlink) {
            if ($r =~ m#^/#) {  # starts with slash, replace entirely
                $_ = &expand_symlink($r);
                s#^/tmp_mnt## && next dir;  # dratted automounter
            } else {            # no slash?  Just replace the tail then
                s#[^/]+$#$r#;
                $_ = &expand_symlink($_);
            }
        }
    }
    s/^\s+//;
    # lots of /../ could have completely emptied the expansion
    return ($_ eq '') ? '/' : $_;
}

use IPC::Open2;

my $cleanup_pid=undef;
my $waitpid_pid=undef;

sub cleanup {
    kill $cleanup_pid if defined $cleanup_pid;
}

#WARNING: returns list - don't use in scalar context blindly
sub exec_filter_list
{
    my ($program,@input)=@_;
    my $f_pid = open2(*FR, *FW, $program);
    die unless $f_pid;
    $waitpid_pid=$cleanup_pid=$f_pid;
    binmode FW;
    print FW $_ for (@input);
    close FW;
    binmode FR;#, ":utf8";
    my @output = <FR>;
#    &debug("Output from $program",@output);
    close FR;
    $cleanup_pid=undef;
    return @output;
}

sub exec_filter
{
    my @list=&exec_filter_list;
    local $,='';
    return "@list";
}

sub exec_exitvalue
{
    die unless defined($waitpid_pid);
    my $ret=waitpid($waitpid_pid,0);
    $waitpid_pid=undef;
    return $ret;
}

#usage: appendzname("a.gz",".suffix") returns "a.suffix.gz",
#appendzname("a",".suffix") return "a.suffix"
sub appendz {
    my ($base,$suffix)=@_;
    my $zext= ($base =~ s/(\.gz|\.bz2|\.Z)$//) ? $1 : '';
    return $base.$suffix.$zext;
}

#usage: $fh=openz($filename); while(<$fh>) ...
sub openz {
    my ($file)=@_;
    my $fh;
    if ($file =~ /\.gz$/) {
        open $fh,'-|','gunzip','-f','-c',$file or die "can't gunzip -c $file: $!"; 
    } elsif ( $file =~ /\.bz2$/) {
        open $fh,'-|','bunzip2','-c',$file  or die "can't bunzip2 -c $file: $!"; 
    } elsif ( $file =~ /^(http|ftp|gopher):/) {
        open $fh,'-|','GET',$file  or die "can't GET $file: $!"; 
    } else {
        open $fh,$file or die "can't read $file: $!";
    }
    return $fh;
}

#usage: &argvz; while(<>) ...
sub argvz() {
    foreach (@ARGV) {
        $_ = "gunzip -f -c ".escape_shell($_)." |" if /\.gz$/;
        $_ = "bunzip2 -c ".escape_shell($_)." |" if /\.bz2$/;
        $_ = "GET ".escape_shell($_)." |" if /^(http|ftp|gopher):/;
    }
}

sub last_file_line() {
    my ($f)=$ARGV;
    $f=~ s/^.*|\S+// if /\.(gz|bz2)$/;
    return ($f,$.);
}

#usage: $fh=openz($filename); print $fh "line\n" ...
sub openz_out_cmd {
    my ($file)=@_;
    my $fh;
    if ($file =~ /\.gz$/) {
        return "|gzip -c > $file";
    } elsif ( $file =~ /\.bz2$/) {
        return "|bzip2 -c > $file";
    } else {
        return ">$file";
    }
}

#usage: $fh=openz($filename); print $fh "line\n" ...
sub openz_out {
    my ($file)=@_;
    my $cmd=&openz_out_cmd($file);
    my $fh;
    open $fh,$cmd or die "can't open $cmd: $!";
    return $fh;
}

sub openz_basename {
    my ($file)=@_;
    $file =~ s/(\.bz2|\.gz)$//;
    return $file;
}


sub expand_tilde {
    my $dir = shift;
    # tilde expansion
    if ($dir =~ /~/) {
	$dir =~ s{ ^ ~( [^/]* ) }
	{ $1 ? (getpwnam($1))[7] : ($ENV{'HOME'} || $ENV{'LOGDIR'} || (getpwuid($>))[7]) }ex;
    }
    return $dir;
}

sub ls_grep {
  my ($dir,$pattern,$include_dir,$hide_dot)=@_;
  $include_dir=1 unless defined $include_dir;
  $hide_dot=0 unless defined $hide_dot;
  $dir='.' unless defined $dir;
  $include_dir=0 if $dir eq '.';
  $pattern='' unless defined $pattern;
  $dir =~ s|/$||;

  -d $dir or die "$dir is not a directory!";
  opendir LS_RECENT,$dir or die $!;
  my @files=grep !/^\.\.?$/, readdir LS_RECENT;
  @files = grep !/^\./, @files if $hide_dot;
  close LS_RECENT;
#  &debug("ls_recent(all):",@files);
  my @filtered=grep /$pattern/, @files;
  @filtered=map {"$dir/$_"} @filtered if $include_dir;
#  &debug("ls_grep(filtered):",@filtered);
  return @filtered;
}

#if no mtime, you don't get it back. (filters for things that have a -M then
#sorts by it)
sub sort_mtime {
    &debug("mtimes",map { ($_,-M) } @_);
    return map { $_->[0] }  # restore original values
      sort { $a->[1] <=> $b->[1] }  # sort
       map { [$_, -M] } (grep { defined -M } @_);  # transform: value, sortkey
}

sub ls_mtime {
  my @sorted=&sort_mtime(ls_grep(@_));
#  &debug("ls_mtime(sorted):",@sorted);
  return @sorted;
}

sub cp_file_check {
    my ($from,$to,@opts)=@_;
    my $absfrom=&abspath_from($from,undef,1);
    my $absto=&abspath_from($to,undef,0);
    return warning("cp_file: source $from(=$absfrom) is the same as destination $to(=$absto)") if ($absfrom eq $absto);
    -f $from or return warning("cp_file: source file $from not readable");
    unlink $to || return warning("cp_file: couldn't remove old destination $to: $?") if -r $to;
    my $cp_bin="/bin/cp";
    $cp_bin="cp" unless -x $cp_bin;
    system $cp_bin,@opts,"--",$from,$to;
    if (my $exit=$?>>8) {
        local $"=' ';
        return warning("$cp_bin from $from to $to with opts ".join(' ',@opts)," failed (EXIT: $?)");
    }
}

sub cp_file_force {
    return cp_file_check(@_) or die "failed to cp_file_check ".join(' ',@_);
}

my $MAXPATH=200; # room for suffixes
sub superchomp {
    my ($ref)=@_;
    $$ref =~ s|^\s+||;
    $$ref =~ s|\s+$||;
    $$ref =~ s|\s+| |g;
}

sub filename_from {
    my ($fname)=@_;
   &superchomp(\$fname);
   $fname =~ s|[^a-zA-Z0-9_-]+|.|g;
   $fname =~ s|^\.|_|;
    return $fname;
}

sub strip_dirnames {
    my ($fname)=@_;
    $fname =~ s|/\S*/([^/ ]+)|$1|g;
    return $fname;
}

### for checkjobs.pl/qsh.pl
sub normalize_jobname {
   my ($fname,$JOBSDIR)=@_;
   $fname=&strip_dirnames($fname);
   $fname=&filename_from($fname);
   my $pathname=substr "$JOBSDIR/$fname",0,$MAXPATH;
   return ($fname,$pathname);
}

sub runtimes {
  my ($user,$sys,$cuser,$csys)=times;
  my $tuser=$user+$cuser;
  my $tsys=$sys+$csys;
  my $ttotal=$tuser+$tsys;
  return "$ttotal wall seconds, $tuser user seconds, $tsys system seconds";
}
###

sub println {
    print @_,"\n";
}

sub read_file_lines_ref {
    my ($filename,$lref)=@_;
    local(*READ_FILE);
    open READ_FILE,"<",$filename || die "couldn't read file $filename: $!";
    @$lref=<READ_FILE>;
    close READ_FILE;
    return scalar @$lref;
}

sub read_file_lines {
    my @ret;
    my ($filename,$singleline)=@_;
    local($/) if $singleline;
    read_file_lines_ref($filename,\@ret);
    return @ret;
}

sub write_file {
    my ($filename,$text,$noclobber,$perm)=@_;
    $noclobber=0 unless defined $noclobber;
    $perm=0644 unless defined $perm;
    local(*WRITE_FILE);
    if (-f $filename) {
        return &warning("$filename already exists") if $noclobber;
        unlink $filename;
    }
    &debug("writing file $filename (text $text)");
    open WRITE_FILE,">",$filename or die "couldn't write to file $filename: $!";
    chmod($perm,$filename) or die "couldn't set permissions on $filename to $perm: $!";
    print WRITE_FILE $text;
    close WRITE_FILE;
    return $filename;
}

sub create_script {
    my ($scriptname,$text,$interp,$noclobber)=@_;
    $interp="/bin/bash" unless defined $interp;
    return write_file($scriptname,"#!$interp\n$text\n",$noclobber,0755);
}

sub quote_list {
    local($")=", ";
    my @ql=map { "q{$_}" } @_;
    return "(@ql)";
}

sub get_blobs {
    $ENV{BLOBS}='/home/hpc-22/dmarcu/nlg/blobs' unless exists $ENV{BLOBS};
    $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
    return $ENV{BLOBS};
}


sub clone_shell_script {
   my ($scriptname,$text,$cd,$bash_exe,$noclobber)=@_;
   $bash_exe=&get_blobs."/bash3/bin/bash" unless defined $bash_exe;
   $bash_exe="/bin/bash" unless -x $bash_exe;
   my $preamble='BLOBS=${BLOBS:-'.&get_blobs.'}'.<<'EOSCRIPT';

[ -d $BLOBS ] || BLOBS=~/blobs
export BLOBS
d=`dirname $0`
if [ -f $BLOBS/bashlib/v8/bashlib.sh ] ; then
 . $BLOBS/bashlib/v8/bashlib.sh
else
 if [ -f $d/bashlib.sh ] ; then
  . $d/bashlib.sh
 fi
fi
EOSCRIPT
   $cd="\ncd ".&getcd."\n" unless defined $cd;
   create_script($scriptname,$preamble.$cd.$text,$bash_exe,$noclobber);
 }

#allincs is boolean.  this file is included, hopefully by absolute path.  if ENV
#were set this would be closer to a continuation of the calling script $cd if
#undef: set cd to current cd when script starts.  (i.e. to not cd, just set to '')
sub clone_perl_script {
    my ($scriptname,$text,$cd,$allincs,$perl_exe,$noclobber)=@_;
    $perl_exe=$^X unless defined $perl_exe;
    $perl_exe = which_in_path($perl_exe) if ($perl_exe !~ m|^/|);
    $allincs='' unless defined $allincs;
    my $incs=quote_list(@INC);
    my ($package, $filename, $line) = eval {caller};
    my $preamble=<<EOSCRIPT;
use strict;
use warnings;
require "$filename";

EOSCRIPT
    my $pwd=&getcd;
   $cd=<<EOSCRIPT unless defined $cd;
chdir q{$pwd} or die "Couldn't cd $pwd: \$?";
EOSCRIPT
    $preamble="push \@INC,$incs;\n$preamble" if $allincs;
    return create_script($scriptname,$preamble . $cd. $text,$perl_exe,$noclobber);
}

#appends filename-safe ARGV of script or ENV{suffix} if set, unless
#ENV{overwrite}, in which case $out is available unmodified usage:
#$cmd="logerr=1 $exec_safe_bin \$out blah blah \$argv" note: $cmd is
#interpolated in double quotes which is why you escape $out and $argv (note:
#$argv is shell-escaped.  also available: ($outdef eq
#$outdirdef.$outfiledef).$suffix eq $outdir.$outfile.  also $in versions: $in
#will be pulled from $ENV{infile} or modified from $ENV{insuffix} (at runtime)
#if available, otherwise the default will be used.
sub perl_shell_appending_argv {
    my ($in,$out,$cmd,$perlcode)=@_;
    $perlcode='' unless defined $perlcode;
    local($")=' ';
    return <<EOSCRIPT;
# perl_appending_argv in=$in out=$out cmd={{{$cmd}}} perlcode={{{$perlcode}}}
{
 my (\$indef)=q{$in};
 my (\$indirdef,\$infiledef)=dir_file(\$indef);
 my \$insuffix=exists \$ENV{insuffix} ?  \$ENV{insuffix} : '';
 my \$in = append_basename(\$indef,\$insuffix);
 \$in = \$ENV{infile} if exists \$ENV{infile};
 my (\$indir,\$infile)=dir_file(\$in);

 my \$outdef=q{$out};
 my (\$outdirdef,\$outfiledef)=dir_file(\$outdef);
 my \$argv=join ' ',\@ARGV;
 my \$suffix=filename_from(\$argv);
 \$suffix=\$ENV{suffix} if exists \$ENV{suffix};
 my \$out=\$ENV{overwrite} ? \$outdef : append_basename(\$outdef,\$suffix);
 my (\$outdir,\$outfile)=dir_file(\$out);
 &mkdir_force(\$outdir);

 &debug("in=\$in out=\$out suffix=\$suffix insuffix=\$insuffix,\$in,\$out");
 $perlcode;
 \$argv=&escaped_shell_args_str(\@ARGV);
 my \$cmd="$cmd";
 &debug("CMD=\$cmd");
 system \$cmd;
 if (my \$exit=\$?>>8) {
  print  STDERR "ERROR: CMD={{{\$cmd}}} FAILED, exitcode=\$exit" ;
  exit \$exit;
 }
}

EOSCRIPT
}

sub abspath_from {
    my ($abspath,$base,$expand_symlink) = @_;
    $expand_symlink=0 unless defined $expand_symlink;
    $base=&getcd unless defined $base;
    $abspath =~ s/^\s+//;
    $base =~ s/^\s+//;
    $abspath =~ s/^\~/$ENV{HOME}/e;
    $base =~ s/^\~/$ENV{HOME}/e;
    if ($abspath !~ m|^/|) {
        &debug("$abspath not absolute, so => $base/$abspath");
        $abspath = "$base/$abspath";
    }
    $abspath =~ s|/\.$||;
    &debug($abspath);
    return &expand_symlink($abspath) if $expand_symlink;
    $abspath =~ s|^\s+||;
    return $abspath;
}


sub inc_numpart {
    my ($text)=@_;
    return ($text =~ s/(\d+)/$1+1/e) ? $text : undef;
}

sub files_from {
    my @files=();
    while(defined (my $f=shift)) {
        while(-f $f) {
            push @files,$f;
            $f=inc_numpart($f);
        }
    }
    return @files;
}

use POSIX qw(HUGE_VAL);
my $INFINITY=HUGE_VAL();

sub getln {
    no warnings 'numeric';
    my ($n)=@_;
    return $n+0 if ($n =~ s/^e\^//);
    return ($n > 0 ? log $n : -$INFINITY);
}

sub getreal {
    my ($n)=@_;
    return exp($1) if ($n =~ /^e\^(.*)$/);
    return $n;
}

sub mean {
    my ($sum,$sumsq,$n)=@_;
    return $sum/$n;
}

sub variance {
    my ($sum,$sumsq,$n)=@_;
    my $v=($sumsq-$sum*$sum/$n)/$n; # 1/n(sumsq)-(mean)^2
    return ($v>0?$v:0);
}

sub stddev {
    return sqrt(&variance(@_));
}

sub stderr {
    my ($sum,$sumsq,$n)=@_;
    return &stddev(@_)/sqrt($n);
}

#returns array of indices from input string of form 1,2,3-10,... (sorted!)
sub parse_range {
    my ($ranges)=@_;
    my @ret;
    if ($ranges ne "") { 
        while ($ranges=~/([^,]+)/g) { # for each comma delimited string
            if ($1 =~ /(\d+)-(\d+)/) { # if it's a range
                warning("parse_range: range $1-$2 empty ($2 < $1)") if $2 < $1;
                push @ret,$_ for ($1 .. $2);
            } else {
                push @ret,$1;
                warning("parse_range: $1 is not a nonnegative integer") unless $1 =~ /^\d+$/;
            }
        }
    }
    return sort @ret;
}

sub parse_comma_colon_hash {
    my $format="expected comma separated key:value pairs, e.g. 'name:Joe,2:3' with no comma or colon appearing in keys or vals";
    my ($hash)=@_;
    my @ret;
    for (split /,/,$hash) {
        /^([^,:]+):([^,:]+)$/ or die "$format - got {{{$hash}}} instead";
        push @ret,$1,$2;
    }
    return @ret;
}

sub filename_from_path_end {
    my($path,$n,$slash_replacement)=@_;
    $n=1 unless defined $n;
    $slash_replacement = '_' unless defined $slash_replacement;
    my @comps=();
    while($path =~ m|([^/]+)|g) {
        push @comps,$1;
    }
    my $start=(scalar @comps)-$n;
    $start=0 if $start<0;
    return join($slash_replacement,map { $comps[$_] } ($start..$#comps));
}

sub mkdir_check {
    my ($dir)=@_;
    my $base='';
    while($dir=~m|(/?[^/]*)|g) {
        $base.=$1;
        &debug("mkdir $base");
        die "Couldn't mkdir $dir because $base is a file" if -f $base;
        mkdir $base unless -d $base;
    }
    system('mkdir','-p','$dir')  unless -d $dir;
    return -d $dir;
}

sub mkdir_force {
    &mkdir_check(@_) or die "Couldn't mkdir @_ $!";
}


# if path=a/b, returns a${suffix}/b.  if path = ./b or b, returns ${suffix}b
sub append_basename {
    my ($path,$suffix)=@_;
    my ($dir,$base)=dir_file_noslash($path);
    return $suffix.$base if ($dir eq '.');
    return "$dir$suffix/$base";
}

my $stderr_replaced=0;
my $stdout_replaced=0;
sub tee_stderr {
    &debug("also writing STDERR to",@_);
    open (SAVE_STDERR, ">&STDERR");
    open (STDERR, "|tee -- @_ 1>&2");   # No use checking for errors (child
                                        # process) 
    &unbuffer(\*STDERR);
    $stderr_replaced=1;
}

sub replace_stderr {
    my ($openstring)=@_;
    open (SAVE_STDERR, ">&STDERR");
    close STDERR;
    $stderr_replaced=1;
    if (open(STDERR,$openstring)) {
        info("Replaced STDERR with $openstring");
    } else {
        &restore_stderr;
        info("Couldn't replace STDERR with $openstring: $! $?");
    }
    set_info_fh(\*STDERR);
}

#program will hang unless you close STDERR after tee_stderr (just call this)
sub restore_stderr {
    if ($stderr_replaced) {
        close (STDERR);
        open (STDERR, ">&SAVE_STDERR");
        close (SAVE_STDERR);
        $stderr_replaced=0;
    }
}

sub tee_stdout {
    open (SAVE_STDOUT, ">&STDOUT");
    open (STDOUT, "|tee -- @_");   # No use checking for errors (child process)
    &unbuffer(\*STDOUT);           # For my use, I wanted un-buffered.
    $stdout_replaced=1;
}

sub replace_stdout {
    my ($openstring)=@_;
    open (SAVE_STDOUT, ">&STDOUT");
    close STDOUT;
    $stdout_replaced=1;
    if (open(STDOUT,$openstring)) {
        info("Replaced STDOUT with $openstring");
    } else {
        &restore_stdout;
        info("Couldn't replace STDOUT with $openstring: $! $?");
    }
}

sub outz_stdout {
    my ($file)=@_;
    replace_stdout(openz_out_cmd($file)) if ($file);
}

sub outz_stderr {
    my ($file)=@_;
    replace_stderr(openz_out_cmd($file)) if ($file);
}

#program will hang unless you close STDOUT after tee_stdout (just call this)
sub restore_stdout {
    if ($stdout_replaced) {
        close (STDOUT);
        open (STDOUT, ">&SAVE_STDOUT");
        close (SAVE_STDOUT);
        $stdout_replaced=0;
    }
}

END {
 &restore_stdout;
 &restore_stderr;
}

#$enc: encoding(iso-8859-7) bytes utf8 crlf etc.
#see http://www.devdaily.com/scw/perl/perl-5.8.5/lib/open.pm.shtml
#should affect stdin/out/err, AND all future opens (not already opened handles)
#crap, doesn't work: use is lexically scoped.
sub set_inenc {
    my ($enc)=@_;
    $enc="bytes" unless defined $enc;
    $enc=":$enc" unless $enc =~ /^:/;
#    use open IN  => $enc;
    binmode STDIN, $enc;
    return $enc;
}

sub set_outenc {
    my ($enc)=@_;
    $enc="bytes" unless defined $enc;
    $enc=":$enc" unless $enc =~ /^:/;
#    use open OUT  => $enc;
    binmode STDOUT, $enc;
    binmode STDERR, $enc;
    return $enc;
}

sub set_ioenc {
    set_inenc(@_);
    return set_outenc(@_);
}

sub require_files {
    -f $_ || die "missing file: $_" for @_;
}

sub require_dirs {
    -d $_ || die "missing file: $_" for @_;
}

sub show_cmdline {
 info("COMMAND LINE:");
 info(&escaped_cmdline);
 info();
}

#TODO: test on LIST/HASH, make a recursive ref_to_string dispatch (see WebDebug.pm)
sub ref_to_string {
    my ($pval,$escape)=@_;
    return 'undef' unless ref($pval);
    my $type=ref($pval);
    return ($escape? escape_shell($$pval) : $$pval) if $type eq 'SCALAR';
    return '('.join ',',@$pval.')' if $type eq 'LIST';
    return '('.join ',',(map {$_ . "=>". $pval->{$_}} keys(%{$pval})).')' if $type eq 'HASH';
    return '[REF: $pval]';
}

use Getopt::Long;

sub getoptions_or_die {
    die "Bad command line options" unless getoptions_catch(@_);
}

my ($getoptions_warn);
sub getoptions_catch {
    $getoptions_warn=undef;
    local $SIG{__WARN__}=sub {$getoptions_warn=$_[0]};
    if (GetOptions(@_) || !defined($getoptions_warn)) {
        return 1;
    } else {
        &show_opts(@_);
        info();
        warning "Error parsing options:\n$getoptions_warn";
        return 0;
    }
}

use English;

our ($option,$description);
#note: this is nicer, but looks terrible with fixed-width font
format OptionDescriptionTabular =
@>>>>>>>>>>>>>>>>>>>>>>>>>>  ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$option,                        $description
                             ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<~~
                                 $description
.

format OptionDescription =
 @<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$option
	^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<~~
    $description
.

sub write_option_description {
    my $tabular=0;
    ($option,$description,$tabular)=@_;
    $FORMAT_NAME=($tabular ? 'OptionDescriptionTabular':'OptionDescription');
    write;
}

format Description =
^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$description
  ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<~~
  $description
.

sub write_description {
    $FORMAT_NAME='Description';
    ($description)=@_;
    write;
}

my $showopts_column=0;
sub set_tabular_show_opts {
    $showopts_column=$_[0];
}
sub show_opts_g {
    my $show_empty=shift;
    my $name=undef;
    my($old) = select($info_fh) if defined $info_fh && ref($info_fh);
#    $FORMAT_NAME='OptionDescription';
    for (@_) {
        if (!defined $name) { # alternate name ...
            $name=$_;
        } else { # and ref ...
            my $val=&ref_to_string($_,1);
            $val='' unless defined $val;
            if ($name ne "help") {
#                $option=$name;
#                $description=$val;
                write_option_description($name,$val,$showopts_column) if $val || $show_empty;
#                write;
#                info(" --$name\t\t$val");
            }
            $name=undef;
        }
    }
    select($old);
}
sub show_opts {
    show_opts_g(0,@_);
}
# takes GetOptions style hash-list (alternating name,ref)
sub show_opts_all {
    show_opts_g(1,@_);
}



#use FileHandle;
my $usage_was_called=0;
# for handling END{} blocks gracefully
sub usage_called() {
    my ($u)=@_;
    my $o=$usage_was_called;
    $usage_was_called=$u if defined $u;
    return $o;
}

sub is_zero {
    return ($_[0] ne "" && ! $_[0]);
}

sub is_numeric {
    no warnings 'numeric';
    return ($_[0] != 0 || &is_zero(@_));
}

sub quote_nonnumeric {
    return is_numeric(@_) ? $_[0] : qq{"$_[0]"};
}

my $tabular_usage=0;
sub set_tabular_usage {
    ($tabular_usage)=@_;
}
# usage: ((["number-lines!",\$num"prepend original line number to output"],[]
# ...))
sub usage_from {
    my @args=@_; # copy so closure below works:
    return sub {
        my ($fh)=@_;
        my $isfh=defined $fh && ref($fh);
        my($old) = select(shift) if $isfh;
        print "NAME\n     $0\n\nUSAGE:\n\n";

        for (@args) {
            if (ref($_) eq 'ARRAY') {
                $option="  --".$_->[0];
                $description=defined $_->[2] ? $_->[2] : '';
                $description .=" (default=".quote_nonnumeric(${$_->[1]}).")" if defined ${$_->[1]};
                write_option_description($option,$description,$tabular_usage);

#                $FORMAT_NAME = "OptionDescription";
#                write;
            } else {
#                $description=$_;
#                $FORMAT_NAME = "Description";
#                write;
                print "\n";
                write_description($_);
            }
        }
        print "\n";
        $description="(note: unambiguous abbreviations like -h are allowed, and boolean options may be prefixed with 'no', e.g. --flag! can be unset by --noflag)";
        write_description($description);
#        $FORMAT_NAME = "Description";
#        write;
        print "\n";
        print "\nBAD COMMAND LINE OPTION: $getoptions_warn\n" if defined $getoptions_warn;
        print "$fh\n" if defined $fh && !$isfh;
        select($old) if defined $old;
        &usage_called(1);
        exit(0);
    }
}

sub opts_from {
    my @opts=();
    for (@_) {
        if (ref($_) eq 'ARRAY') {
            push @opts,($_->[0],$_->[1]);
        }
    }
    return @opts;
}

#returns usage sub ref then all the GetOptions style opts (for show_opts)
sub getoptions_usage {
    my @opts=opts_from(@_);
    my $usage=&usage_from(@_);
    push @opts,("help"=>$usage);
    unless (getoptions_catch(@opts)) {
        $usage->();
    }
    return ($usage,@opts);
}

#returns usage sub ref then all the GetOptions style opts (for show_opts)
sub getoptions_usage_verbose {
    my $cmdline=&escaped_cmdline;
    my($old) = select($info_fh) if defined $info_fh && ref($info_fh);
    my ($usage,@opts)=&getoptions_usage(@_);
    print "<<<<<< PARAMETERS:\n";
    print "CWD: ",&getcd,"\n";
    print "COMMAND LINE: $cmdline\n\n";
    show_opts(@opts);
    print ">>>>>> PARAMETERS\n\n";
    select($old);
    return ($usage,@opts);
}

sub sort_num {
    return sort { $a <=> $b } @_;
}

my $EPSILON=0.001;

sub set_epsilon {
    my $oldeps=$EPSILON;
    $EPSILON=$_[0] if defined $_[0];
    return $oldeps;
}
sub epsilon_equal {
    return abs($_[1]-$_[0])<= $EPSILON;
}

sub epsilon_greater {
    return $_[0]>$_[1]+$EPSILON;
}

sub epsilon_lessthan {
    return $_[1]>$_[0]+$EPSILON;
}

#useful for more efficient sorting on val when key is long
sub push_pairs_hash_to_list {
    my ($href,$lref)=@_;
    while (my ($k,$v)=each %$href) {
        push @$lref,[$k,$v];
    }
}

#subst("aBc", qr/([a-z]+)/ => sub { uc $_[1] }, qr/([A-Z]+)/ => sub { lc$_[1] }, ) ;
#subst("aBc", qr/([a-z]+)/ => '$1$1')
sub subst
{
    local $_;
    my $str = shift;
    my $pos = 0;
    my @subs;
    while (@_) {
        push @subs, [ shift, shift ];
    }
    my $res;
    while ($pos < length $str) {
        my (@bplus, @bminus, $best);
        for my $rref (@subs) {
            pos $str = $pos;
            if ($str =~ /\G$rref->[0]/) {
                if (!defined $bplus[0] || $+[0] > $bplus[0]) {
                    @bplus = @+;
                    @bminus = @-;
                    $best = $rref;
                }
            }
        }
        if (@bminus) {
            my $temp = $best->[1];
            if (ref $temp eq 'CODE') {
                $res .= $temp->(map { substr $str, $bminus[$_], $bplus[$_]-$bminus[$_] } 0..$#bminus);
            }
            elsif (not ref $temp) {
                $temp = subst($temp, 
                              qr/\\\\/        => sub { '\\' },
                              qr/\\\$/        => sub { '$' },
                              qr/\$(\d+)/     => sub { substr $str, $bminus[$_[1]], $bplus[$_[1]]-$bminus[$_[1]] },
                              qr/\$\{(\d+)\}/ => sub { substr $str, $bminus[$_[1]], $bplus[$_[1]]-$bminus[$_[1]] },
                        );
                $res .= $temp;
            }
            else {
                die 'Replacements must be strings or coderefs, not ' . ref($temp) . ' refs';
            }
            $pos = $bplus[0];
        }
        else {
            $res .= substr $str, $pos, 1;
            $pos++;
        }
    }
    return $res;
}

my $default_template='<{[()]}>';

#expand_template(121212,1,3,6) => 32622 and an uninit value warning.
sub restore_extract {
    my ($text,$template,@vals)=@_;
    $template=$default_template unless defined $template;
    my $i;
    return subst($text,qr/\Q$template\E/,sub { $vals[$i++] });
}

# number is loosely defined, with -+.^eE getting stolen unless protected by
# word boundaries and e^number as a legal alternative (log scale) rep.
sub extract_numbers {
    my ($text,$template)=@_;
    $template=$default_template unless defined $template;
    my @nums=();
    my $loose_num_match=qr/((?:[+\-]|\b)[0123456789.][eE^0123456789.\-+]*)\b/;
    push @nums,$1 while ($text=~s/$loose_num_match/$template/);
    return ($text,@nums);
}

my %summary_list_n_sum_max_min;

sub get_number_summary {
    return \%summary_list_n_sum_max_min;
}

#list: # of events,sums,(TODO:maxes)
sub add_to_n_list {
    my ($ppOld,@nums)=@_;
    my $pOld=$$ppOld;
    if (!defined $pOld) {
        $$ppOld=[1,[@nums],[@nums],[@nums]];
    } else {
        $pOld->[0]++;
        my $pSum=$pOld->[1];
        my $pMax=$pOld->[2];
        my $pMin=$pOld->[3];
        local $_;
        for (0..$#nums) {
            $pSum->[$_]+=$nums[$_];
            $pMax->[$_]=$nums[$_] unless $pMax->[$_] > $nums[$_];
            $pMin->[$_]=$nums[$_] unless $pMin->[$_] < $nums[$_];
        }
    }
}

sub print_number_summary {
    return unless scalar keys %summary_list_n_sum_max_min;
    my ($fh)=@_;
    $fh=$info_fh unless defined $fh;
    my($old) = select($fh);
    my @temps=sort keys %summary_list_n_sum_max_min;
    print "NUMERICAL DIGEST - \"avg(N=<n-occurences>): text-with-{{{min/avg/max}}}-replacing-numbers\":\n";
    for (@temps) {
        my ($n,$pSum,$pMax,$pMin)=@{$summary_list_n_sum_max_min{$_}};
        my $restore_avg=restore_extract($_,undef,map {"{{{$pMin->[$_]/".sprintf("%.8g",$pSum->[$_]/$n)."/$pMax->[$_]}}}"} (0..$#$pSum));
        print "avg(N=$n): $restore_avg\n";
    }
    select($old);
}

sub remember_numbers {
    my ($text,@nums)=@_;
    &debug("remember",$text,@nums);
    add_to_n_list(\$summary_list_n_sum_max_min{$text},@nums);
}

sub log_numbers {
    my $text="@_";
    chomp $text;
    &debug("log_number_summary",$text);
    &debug(extract_numbers($text));
    remember_numbers(extract_numbers($text));
}


sub push_hash_list {
    my ($ppOld,@nums)=@_;
    if (!defined $ppOld) {
        $$ppOld=[@nums];
    } else {
        push @$$ppOld,@nums;
    }
}

#regexp: $1 = fieldname, $2 = {{{attr-with-spaces}}} or attr, $3 =
#attr-with-spaces, $4 = attr
sub getfield_regexp {
    my ($fieldname)=@_;
    $fieldname = defined $fieldname ? qr/\Q$fieldname\E/ : qr/\S+/;
    return qr/\b(\Q$fieldname\E)=({{{(.*?)}}}|([^{]\S*))\b/;
}

sub getfield {
  my ($field,$line)=@_;
  if ($line =~ /\Q$field\E=(?:{{{(.*?)}}}|(\S*))/) {
      return (defined $1) ? $1 : $2;
  } else {
      return undef;
  }
}


sub single_quote_interpolate {
    my ($string)=@_;
    my $quoted=q{"$string"};
    my $evaled=eval $quoted;
    return $evaled ? $evaled : $string;
}

sub double_quote_interpolate {
    my ($string)=@_;
    my $quoted=qq{"$string"};
    my $evaled=eval $quoted;
    return $evaled ? $evaled : $string;
}

#if @opts=(["a",\$a],...)
#usage: expand_opts(\@opts,["before","after"])
sub expand_opts {
    my $ref_opts_usage=shift;
      for (@$ref_opts_usage) {
          if (ref($_) eq 'ARRAY') {
              my ($name,$ref)=@$_;
              if (ref($ref) eq 'SCALAR' && defined $$ref) { 
                  my $old_val=$$ref;
                  for (@_) {
                      $$ref =~ s/\Q$_->[0]\E/$_->[1]/g;
                  }
                  info("Expanded $name from $old_val to $$ref") unless $old_val eq $$ref;
              }
          }
      }
}

sub system_postmortem {
    if ($? == -1) {
        return "failed to execute: $!";
    } elsif ($? & 127) {
        return sprintf("child died with signal %d, %s coredump",
          ($? & 127),  ($? & 128) ? 'with' : 'without');
    } elsif ($? >> 8) {
        return sprintf("child exited with value %d\n", $? >> 8);
    }
    return '';
}

sub system_postmortem_assert {
    my $result=&system_postmortem;
    die $result if $result;
}

sub send_email {
    my ($text,$subject,@to)=@_;
    local(*EMAIL);
    open(EMAIL,'|mail -s "'.$subject.'" '.join(' ',@to)) or die "Couldn't send email - $!: $?";
    print EMAIL $text,"\n";
    close EMAIL;
}

sub symlink_update {
    my ($source,$dest)=@_;
    unlink $dest if readlink($dest);
    symlink($source,$dest);
}

sub read_srilm_unigrams {
    my ($fh,$hashref)=@_;
    local $_;
    my $uni=0;
    while(<$fh>) {
        if (/^\\1-grams:$/) {
            &debug("expecting 1-grams: $. $_");
            $uni=1;
        } elsif (/^\\.*:$/) {
            &debug("end of 1-grams: $. $_");
            return;
        } elsif ($uni) {
#            &debug("trying to parse unigram",$_);
            if (/^(\S+)\s+(\S+)/) {
                my ($prob,$word)=($1,$2);
                $hashref->{$word}=$prob;
#                &debug("parsed srilm unigram",$word,$prob);
            }
        }
    }
}

sub log10_to_ln {
    return $_[0]*log(10);
}

my $default_precision=13;
sub set_default_precision {
    $default_precision=defined $_[0] ? $_[0] : 5;
}

sub real_prec {
    my ($n,$prec)=@_;
    $prec=$default_precision unless defined $prec;
    sprintf("%.${prec}g",$n);
}

sub log10_to_ln_prec {
    my ($log10,$prec)=@_;
    return real_prec(log10_to_ln($log10));
}

sub log10_to_ehat {
    return "e^".&log10_to_ln_prec;
}

sub real_to_ehat {
    return "e^".real_prec(&getln);
}

# nearest_two_linear($x,\@B) returns ($i,$a) such that $B[$i]*(1-$a) + $B[$i+1]*$a == $x
sub nearest_two_linear {
    my ($x,$R)=@_;
#    &debug("nearest_two_linear",$x,$R->[0]);
    return (0,0) if ($x < $R->[0]);
    my $LAST=$#$R; # last index
    for (0..$LAST-1) { #FIXME (in theory): binary search, blah blah - not using
                    #large lists anyway
        my $b=$R->[$_+1];
#        &debug("if $x <= $b...");
        if ($x <= $b) {
            my $a=$R->[$_];
#            &debug(" $x between $a and $b at index $_");
            return ($_,($x-$a)/($b-$a)); # fraction of the way from b to c (0
                                         # means you're on $_
        }
    }
    return ($LAST,0),
}

sub escape_3brackets {
    local($_)=@_;
    return "{{{$_}}}" if (/\s/);
    return $_;
}

sub read_hash {
    my ($file,$href,$invhref)=@_;
    local *HASH;
    open HASH,'<',$file or die "Couldn't read file $file: $!";
    local $_;
    while(<HASH>) {
        chomp;
        my ($f1,$f2)=split;
        $href->{$f1}=$f2 if defined $href;
        $invhref->{$f2}=$f1 if defined $invhref;
    }
    close HASH;
}

sub hash_lookup_default {
    my ($hashref,$key,$default)=@_;
    return (exists $hashref->{$key}) ? $hashref->{$key} : $default;
}

1;

