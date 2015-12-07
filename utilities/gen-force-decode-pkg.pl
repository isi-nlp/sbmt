#!/usr/bin/env perl

# from Michael. Uses an ApplicableRuleFilter that requires 
# some packages in ~voeckler/lib/perl, so PERL5LIB must be set to that
# My adaptations: allow original tree format to be read in.

BEGIN {
    use FindBin qw($Bin);
    # $platform looks unnecessary... --michael
    # my $platform = `uname -i`;
    # $platform =~ s/^\s+//;
    # $platform =~ s/\s+$//;
    push @INC, $Bin;
}

my $xrsdb_batch_retrieval="xrsdb_batch_retrieval";

my $DEBUG=0;

sub set_debug { $DEBUG=1; arf_set_debug(1); }

my $PARALLEL_BINARCH=0;
sub set_parallel { $PARALLEL_BINARCH=1; }

sub debug_print {
    print STDERR @_ if $DEBUG;
}

# usage: from_xrsdb(\@farray,\@earray,$startid,$xrsdb,$archive_path)
sub from_xrsdb {
    my $farray = shift;
    my $earray = shift;
    my $n = shift;
    my $xrsdb = shift;
    my $archive_path = shift;
    my $f;
    my $e;
    open $f, ">tmp_f.$n";
    open $e, ">tmp_e.$n";
    print $f @$farray;
    print $e @$earray;
    `$xrsdb_batch_retrieval -i tmp_f.$n -e tmp_e.$n -u force_etree -n $n -p $archive_path/ -s .rules -d $xrsdb`;
    unlink $f;
    unlink $e;
}

sub from_applicable_rule_filter {
    my $farray = shift;
    my $earray = shift;
    my $startid = shift;
    my $grammar = shift;
    my $archive_path = shift;
    
    my $s = $startid;
    foreach my $f (@$farray) {
        # for each sentence a stream is created that pipes applicable rules through
        # itg_binarizer | unknown_word_rules | archive_grammar
        # possibly for large chunks this could be too much of a resource drain?			       
        my $fh;
        open $fh, ">$archive_path/$s.rules" or die "Can't open $archive_path/$s.rules: $!\n";  
        $xrsfile[$s] = $fh; 
        $s++;
    }
    
    my $farf = ftarf_create($earray,$farray);
    print STDERR "Done with filter creation\n";
    if ($grammar =~ /\.gz$/) { 
        open(GRAM, "gunzip -c $grammar|") or die "Couldn't open $grammar";
    } else { 
        open(GRAM, "$grammar") or die "Couldn't open $grammar";
    }
    
    my $ruleid = 0;
    while (<GRAM>) {
        $ruleid++;
        my $rule = $_;
        my @rule_matches = ftarf_all_matches($farf,$rule);
        my $sentid = $startid;
        debug_print "@rule_matches \n";
        foreach my $match (@rule_matches) {
            eval {
                if ($match) {
                    debug_print "matches sentence $sentid";
                    print { $xrsfile[$sentid] } $rule;
                }
            }; die $@ . "sentid:$sentid and rule $rule" if $@;
            $sentid++;
        }
        if ($ruleid % 10000 == 0) {
            print STDERR "($start-$end): Processed rule $ruleid\n";
        }
    }
    
    close GRAM;
    foreach my $id ($start .. $end) {
        print STDERR "Closing file $id\n";
        my $fh = $xrsfile[$id];
        if (defined($fh)) { $fh->close(); }
        unlink "$archive_path/$id.sent"; 
    }
}

use strict;
use Errno qw(EAGAIN);
use Cwd;
use Getopt::Long;
use ApplicableRuleFilter;
use File::Path;
use File::Basename;
use IO::File;
use POSIX qw(ceil);


sub usage {
    print STDERR "\n";
    print STDERR "help       -- print this message\n";
    print STDERR "archive    -- output basename (.tar.gz will be appended)\n";
    print STDERR "grammar    -- input grammar. not using xrsdb\n";
    print STDERR "xrsdb      -- location of xrsdb root directory. not using grammar\n";
    print STDERR "xrsdb-bin  -- full path name of xrsdb_batch_retrieval program\n";
    print STDERR "source     -- source sentences\n";
    print STDERR "target     -- target sentences\n";
    print STDERR "maxwords   -- maximum sentence (source) length; default is unlimited\n";
    print STDERR "maxwordsfile -- file to write lines that have been ditched for maxwords; default is /dev/null\n";
    print STDERR "debug      -- turn on debug messages\n";
    print STDERR "parallel   -- turn on parallel binarch\n";
    print STDERR "bin-path   -- location of archive_grammar, itg_binarizer,\n"
               . "              if different than location of this script\n";
    print STDERR "force-etree -- target-constraints should be trees\n";
    exit;
}

my $grammar;
my $source;
my $target;
my $archive_path="./sbmt-pkg";
my $bin_path=$FindBin::Bin;
my $force_tree=0;
my $max_sent_length=0;
my $max_words_file="/dev/null";
my $xrsdb="";
my $using_xrsdb=0;

sub set_xrsdb {
    shift;
    $xrsdb=shift;
    $using_xrsdb=1;
    print STDERR "using xrsdb=$xrsdb\n";
}

GetOptions(  "help"        => \&usage
           , "archive:s"   => \$archive_path
           , "grammar:s"   => \$grammar
           , "xrsdb:s"     => \&set_xrsdb
           , "xrsdb-bin:s" => \$xrsdb_batch_retrieval
           , "source=s"    => \$source
           , "target=s"    => \$target
           , "maxwords:i"  => \$max_sent_length
           , "maxwordsfile:s"  => \$max_words_file
           , "debug"       => \&set_debug
           , "parallel"    => \&set_parallel
           , "bin-path:s"  => \$bin_path 
           , "force-etree" => sub {$force_tree=1;}
);

if ($bin_path eq "") { $bin_path=$FindBin::Bin; }

my ($archive_name,$archive_dir) = fileparse($archive_path);

debug_print ("archive-package will be $archive_dir/$archive_path.tar.gz\n");
  
# my $wd = cwd();

mkpath $archive_path 
  or  (rmtree $archive_path and mkpath $archive_path) 
  or die "couldnt create archive-package dir $archive_path";

open (SRC, $source) or die "couldnt open source file $source";
my @source_sentences = <SRC>;
close SRC;

open (TGT, $target) or die "couldnt open target file $target";
my @target_sentences = <TGT>;
close TGT;

# how many per parallel batch?
# 1) how many sentences? 
my $srcsize = scalar(@source_sentences);
if ($srcsize != scalar(@target_sentences)) {
    die "Weird: $source has $srcsize lines, but $target has ".scalar(@target_sentences)."\n";
}

# 2) how many cpus?
# added small app to print out cpu count --michael
my $cpucount = `$bin_path/numcpu`;
#my $cpucount = `grep -c vendor_id /proc/cpuinfo`;
chomp $cpucount;
if ($cpucount < 1 || $cpucount > 100) {
    die "Weird value for cpu count: $cpucount\n";
}

my $BATCH_LIMIT = ceil($srcsize/$cpucount);
print STDERR "$srcsize lines over $cpucount cpus; $BATCH_LIMIT sentences per cpu\n";

# convert the trees
# if a sentence is empty, make a placeholder
map { $_ =~ s/~\d+~\d+//g; $_ =~ s/\s+[-\d\.]+\s+/ /g; if ($_ !~ /\(/) { $_ = "(TOP (NO PARSE) )\n";}} @target_sentences;

# filter long sentences
unless ($max_sent_length < 1) {
    my $overflow = $max_sent_length+1;
    open MAXWORDS, ">$max_words_file" or die "Couldn't open $max_words_file: $!\n";
    for (my $i = 0; $i < scalar(@source_sentences); $i++) {
	my $sentnum = $i+1;
	my $size = scalar(split ' ', $source_sentences[$i], $overflow);
	if ($size >= $overflow) {
	    $target_sentences[$i] = "(TOP (OVERFLOW REACHED) )\n";
	    $source_sentences[$i] = "overflow\n";
	    print MAXWORDS "$sentnum\n";
	}
    }
}
    

my @xrsfile;

my $force_tree_cmd;
if ($force_tree) { $force_tree_cmd = "--etree-string"; }
else { $force_tree_cmd = ""; }

my $start = 1;
my $end = $BATCH_LIMIT;
if (@source_sentences < $end) {
    $end = @source_sentences;
}

# Fork each of these batches!!
my @procs;
while ($start <= @source_sentences) {
    FORK : {
	if (my $pid = fork) {
	     # parent
	    print STDERR "Launched proc $pid to go from $start to $end\n";
	    push @procs, $pid;
	    $start = $end+1;
	    $end += $BATCH_LIMIT;
	    if (@source_sentences < $end) {
		$end = @source_sentences;
	    }
	    next;
	}
	elsif (defined $pid) {
	    
	    print STDERR "Forked job processing from $start to $end\n";
	    
	    my @subtarget = @target_sentences[($start-1)..($end-1)];
	    my @subsource = @source_sentences[($start-1)..($end-1)];
	    
        if ($using_xrsdb) {
            from_xrsdb(\@subsource,\@subtarget,$start,$xrsdb,$archive_path);
        } else {
            from_applicable_rule_filter(\@subsource,\@subtarget,$start,$grammar,$archive_path);
        }

	    print STDERR "($start-$end): Done!\n";
	    if ($PARALLEL_BINARCH) {
		print STDERR "Doing binarizer/archiver in parallel\n";
		foreach my $id ($start .. $end) {
		    my $rawfile = "$archive_path/$id.rules";
		    open RAW, "$rawfile" or die "Can't open $rawfile: $!\n";
		    my $binarch = new IO::File "| $bin_path/itg_binarizer --left-right-backoff $force_tree_cmd " 
			. "| $bin_path/archive_grammar -o $archive_path/$id.gar";
		    while (<RAW>) {
			print $binarch $_;
		    }
		    close $binarch;
		    unlink $rawfile;
		    print STDERR "Done with binarization and archiving of $id\n";
		}
	    }
	    exit(0);
	}
	elsif ($! == EAGAIN) {
	    print STDERR "Fork error; trying again\n";
	    sleep 5;
	    redo FORK;
	}
	else {
	    die "Can't fork: $!\n";
	}
    }
}

# wait for them to finish
foreach my $proc (@procs) {
    print STDERR "Waiting for $proc\n";
    my $id = waitpid($proc,0);
    print STDERR "Got it: $id\n";
}

unless ($PARALLEL_BINARCH) {
    print STDERR "Doing binarizer/archiver linearly\n";
    # now do all the binarization/archiving
    foreach my $id (1 .. scalar(@source_sentences)) {
	my $rawfile = "$archive_path/$id.rules";
	open RAW, "$rawfile" or die "Can't open $rawfile: $!\n";
	my $binarch = new IO::File "| $bin_path/itg_binarizer --left-right-backoff $force_tree_cmd " 
	    . "| $bin_path/archive_grammar -o $archive_path/$id.gar";
	while (<RAW>) {
	    print $binarch $_;
	}
	close $binarch;
	unlink $rawfile;
	print STDERR "Done with binarization and archiving of $id\n";
    }
}
# writing out the instruction-file to be passed to the decoder.
# looks something like:
#
# load-grammar "1.gar";
# decode <foreign-sentence> A B C
#
# load-grammar "2.gar";
# decode <foreign-sentence> A B C
#
open (INS, "> $archive_path/decode.ins");
foreach my $id (1 .. scalar(@source_sentences)) {
    print INS "load-grammar archive \"$id.gar\";\n";
#    print INS "load-grammar archive \"$id.gram\";\n";
    print INS "force-decode {\n"; 
    my $sent = $source_sentences[$id-1];
    print INS "\tsource: $sent \n";
    $sent = $target_sentences[$id-1];
    if (!$force_tree) { $sent = etree_sentence($sent); }
    print INS "\ttarget: $sent \n";
    print INS "}\n\n";
}

close INS;

# foreach my $fh (@xrsfile) {
#     if (defined($fh)) { $fh->close(); }
# }

# foreach my $id (1 .. scalar(@source_sentences)) {
#     unlink "$archive_path/$id.sent"; 
# }
# chdir($archive_dir);

# if (-e "$archive_name.tar.gz") { unlink "$archive_name.tar.gz"; }
# `tar -cz $archive_name > $archive_name.tar.gz`;

# chdir($wd);

# rmtree $archive_path;
