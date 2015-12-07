#!/usr/bin/env perl
#
# Author: ithayer,graehl
# Created: Thu Sep 23 08:14:52 PDT 2004
#
# This script extracts the 1 best hypothesis from our decoder log file
#

#TODO: nbests extract/fallback also (for tuning)

use strict;
use warnings;
use Getopt::Long;

### script info ##################################################
use File::Basename;
my $scriptdir;			# location of script
my $scriptname;			# filename of script
my $BLOBS;

BEGIN {
    $scriptdir = &File::Basename::dirname($0);
    ($scriptname) = &File::Basename::fileparse($0);
    push @INC, $scriptdir; # now you can say: require "blah.pl" from the current directory
    $ENV{BLOBS}='/home/nlg-01/blobs' unless exists $ENV{BLOBS};
    $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
    $BLOBS=$ENV{BLOBS};
    my $libgraehl="$BLOBS/libgraehl/v9";
    push @INC,$libgraehl if -d $libgraehl;
    my $decoder="$BLOBS/decoder/v11";
    push @INC,$decoder if -d $decoder;
}

require "libgraehl.pl";
require "libdecoder.pl";

### arguments ####################################################
&usage() if (($#ARGV+1) < 1);	# die if too few args

my $infile_name;		# name of input file
my $keep_unknown=0;
my $just_parsed=0;
my $rtfile="";			# replacement translations
my $prev_over_rt=0;
my $ignore;
my $max_sents=99999999;
my $min_sents=1;
my $translate_range=undef;
my $reorder_lines=1;
my $missing_log='';
my $found_log='';
my $logfile_name='';
my $outfile_name='';
#my $summary_logfile_name='';

my $max_orig_rank=1;
my $sum_orig_rank=0;
my $N=0;
my $nbest_limit=1;
my $nbest_limit_reached=0;
my $raw_nbests=0; # currently handled in output_hyp (replacing $hyp with $_)

my $pass_name;

$ARGV[$_] =~ s/^--translate-range/--translate-lines/ for (0..$#ARGV);


my @opts_usage=("Expects nbest lists (or failure messages), which must have unique sent=N labels, producing a one-per-line hypothesis file, and a missing-log explaining what happened for sentences that didn't have nbests.",
                ["raw-nbests!" => \$raw_nbests,"don't extract just the hyp part - keep the whole nbest line(s)"],
                ["infile=s" => \$infile_name,"(optional) - take <file> as input (as well as the rest of ARGV)"],
                ["outfile=s" => \$outfile_name,"(optional) write output here AS WELL AS stdout"],
                ["logfile=s" => \$logfile_name,"(optional) write diagnostic messages here (as well as STDERR)"],
                ["translate-lines=s" => \$translate_range,"Skip sentences that don't occur in this range (e.g. 1-5,9,10,12), and also complain (use --rt-file) if their nbests are missing"],
                ["rt-file=s" => \$rtfile,"retry file - expect a single translation one per line, starting with sent=1 - to be used if the input lacks that sentence"],
                ["prefer-prev-pass!" => \$prev_over_rt,"Use previous pass nbest instead of --rtfile if possible."],
                ["min-sents-output=i" => \$min_sents,"Pad output to include blanks if less than i sentences are found"],
                ["missing-log=s" => \$missing_log,"Write a log of anomolous/missing sentences here"],
                ["found-log=s" => \$found_log,"Write a log of completely succesful sentences here"],
                ["max-sents=i" => \$max_sents,"Immediately exit after i sentence bests are found (can speed things if you know you're only submitting one sentence)"],
                ["reorder-lines!" => \$reorder_lines,"Sort outputs by sent=N"],
                ["keep-unknown!" => \$keep_unknown,'normally you want @UNKNOWN@ stripped for bleu scoring, but this will leave them in'],
                ["just-parsed!" => \$just_parsed,"(deprecated) only print succesful parses"],
		"UNIMPLEMENTED:",
                ["n-best-length:i" => \$nbest_limit,"keep multiple nbests (from nbest=0...i-1, inclusive) - UNIMPLEMENTED"],
		#                ["summary-log=s" => \$summary_logfile_name,"(unimplemented) record interesting aggregate sums/averages over summaries from decoder logs here"],
                ["pass=s" => \$pass_name,"extract only nbests for (pass s) instead of final-pass"],

		);

my $cmdline=&escaped_cmdline;
my ($usageproc,@opts)=getoptions_usage(@opts_usage);
&tee_stderr($logfile_name) if $logfile_name;

info("COMMAND LINE:");
info($cmdline);
show_opts(@opts);


### main program ################################################
my $prefpass='';
if ($pass_name) {
    $prefpass="\Q(pass $pass_name)\E";
}

if ($infile_name) {
    info("Adding infile = $infile_name");
    unshift @ARGV,$infile_name;
}

&set_ioenc("bytes"); # or encoding(gb..something)? #lexicaly scoped so ...
use open IO => ':bytes';
&tee_stdout($outfile_name) if $outfile_name;

&argvz;
#binmode;
my $sent;			# holds last seen sent
my $old_sent;

my %hyps=();   # key: sent=I val: '' for missing or expected translation
my @range_sents=();
my %range_hyps=();

@range_sents=parse_range($translate_range) if defined $translate_range;
&debug(@range_sents);
$range_hyps{$_}=$hyps{$_}='' for @range_sents;

sub expect_sent {
    my ($sent)=@_;
    $hyps{$sent}='';
    &debug("expecting sent:",$sent);
}

# $hyp has newline.
sub enter_sent {
    my ($sent,$hyp)=@_;
}

my @rts=();			# replacement translations
if ($rtfile ne "") {	  # if replace ment translation file specified, 
    if (open(RT, "$rtfile")) {
	@rts = <RT>;
	close(RT);
    } else {
	warning("Couldn't open retry file $rtfile");
	$rtfile="";
    }
}

#1 indexed line in retry file
sub getrt {
    my ($i) = @_;
    --$i;
    if ($i > $#rts || !defined $rts[$i]) {
	warning("Couldn't get line $i from retry file $rtfile");
	return "";
    }
    my $ret=$rts[$i];
    chomp $ret;
    return $ret;
}

my $nparsed=0;
my $unknown_words=0;
my $extracted="";

my $sentence_count=0;
my $null_trans=0;
my $prev_pass='';
my $nhyp=0;

open MISSING,'>',$missing_log if $missing_log;
my %dispositions;
sub submit_disposition {
    my ($sent,$disposition)=@_;
    $sent='unknown' unless defined $sent;
    if ($disposition) {
	my $msg="sent=$sent $disposition";
	info($msg);
	print MISSING "$msg\n" if $missing_log;
    }
    $dispositions{$disposition}++;
}

#hyp may or may not have newline
sub output_hyp {
    info("START output_hyp");
    my ($hyp,$disposition,$sent)=@_;
    # my @called=caller;
    #    &debug("output_hyp CALLED: ",@called,"hyp=$hyp sent=$sent disposition=$disposition line=$_");
    if ($raw_nbests && $hyp) {
	$hyp=$_;
    }
    if ($hyp) {
	#/unreranked-n-best-pos=(\d+)/ )
	#my $nbestline=$_;
	# 2008-10-28 (jsv): prefix with space to proper terminate\
	#$nbestline =~ s/ (hyp|tree|derivation)={{{.*?}}}/ <<<$1>>>/g;
	#log_numbers($nbestline);
    }
    $disposition='' unless defined $disposition;

    while ($hyp =~ /\@UNKNOWN\@/g) {
	$unknown_words++;
    }

    # 2008-10-28 (jsv): some updates, checked with JG
    $hyp =~ s/\@UNKNOWN\@//g unless $keep_unknown;
    $hyp =~ s/\s+/ /g;		# collapse multi-ws
    $hyp =~ s/^ +//; 		# trim front
    $hyp =~ s/ +$//; 		# trim rear, keep LF?

    ++$nhyp;
    #    $sent=$nhyp unless defined $sent;
    chomp $hyp;
    if (defined $sent) {
	if ($hyps{$sent}) {
	    goto done_hyp unless $hyp;
	    warning("More than one hypothesis for sent=$sent: overwriting with the new \"$hyp\"");
	}
	unless ($hyp) {
	    if ($rtfile) {
		$hyp=&getrt($sent);
		$disposition .= ", fell back to --rt-file \"$rtfile\"" if $hyp;
	    }
	}
	info("USING: hyp=$hyp sent=$sent",($disposition?" disposition=$disposition":''));
	$hyps{$sent}=$hyp;
    }
    submit_disposition($sent,$disposition);
    if (!$reorder_lines) {
	print "$hyp\n" unless (scalar @range_sents && defined $sent && $range_hyps{$sent} ne '');
	$range_hyps{$sent} = $hyp if defined $sent;
    }
    if ($nhyp >= $max_sents) {
	print STDERR "exiting early because $nhyp have been printed and max-sents $max_sents was specified\n";
	exit 0;
    }
    info("END");
  done_hyp:
    $old_sent=undef;

}
my $nprev_pass=0;
my $saved_line=undef;
my $done=0;
my $line_type;
while (1) {
    last if $done;
    $_ = defined $saved_line ? $saved_line : <>;

    if (defined $_) {
	chomp;
	if ($nbest_limit == 1) {
	    next if / nbest=[^0]/; #optimization: don't care about nbests != 0
	}
	$line_type=decoder_log_type($_);
	log_numbers($_) if ($line_type eq 'summary');
    }
    ### somewhat complicated logic to handle end of file without having seen
    ### expected nbest
    $sent=$1 if (defined $_ && /\bsent=(\d+)\b/);
    if (defined $old_sent && (!defined $_ || $old_sent != $sent)) {
	$saved_line=$_;
	if ($prev_pass) {
	    $nbest_limit_reached=0;
	    $old_sent=$sent;
	    &debug("sent=$sent changed, going to prev_pass");
	    $done=1 unless defined $_;
	    goto prevpass;
	}
	&output_hyp("","sent=N changed without seeing any nbests",$old_sent);
	#        info("sent=$sent seen before any hypothesis for old sent=$old_sent");
	$sentence_count++;
    }
    ### done: onto normal eof handling:

    last unless defined $_;
    $saved_line=undef;

    ##done: same as <> but with $saved_line putback buffer

    if (/(\d+) lines to translate:(.*)$/) {
	my ($total,$linelist)=($1,$2);
	info("From decoder log intro: expecting $1 total sents: $2");
	&expect_sent($_) for split(' ',$linelist);
    }
    if (/Decoding sent=(\d+)/) {
	$sent=$1;
	info("From decoder log: decoding sent=$sent");
	&expect_sent($sent);
    }
    $old_sent=$sent if defined $sent;
    my $passn='(\(pass \d+\) )?';
    my $recovered=/^RECOVERED.*hyp={{{.*?}}}/;
    my $failed=/^R(?:ECOVERY|ecovery) failed/i;
    my $exception=/st9bad_alloc/;
    if ($exception or $recovered or $failed or  /^${passn}NBEST sent=(\d+) nbest=/o ) {
	next if (/nbest=(\d+)/ && $1 >= $nbest_limit);
	my $pass=0;
	if (/^\(pass (\d+)\) /) {
	    $pass=$1;
	    $prev_pass=$_;
	    &debug($prev_pass);
	    next;
	}
	#POST: we're only here (ONCE) for final pass stuff.


      again:
	if (/^${prefpass}NBEST sent=(\d+) nbest=(\d+)/ && $2 < $nbest_limit) {
	    $prev_pass='';
	    $sent=$1;
	    if (/\bunreranked-nbest-pos=(\d+)\b/) {
		my $unpos=$1;
		$max_orig_rank = $unpos if ($unpos > $max_orig_rank);
		$sum_orig_rank += $unpos;
		++$N;
	    }
	    $nparsed++ unless $pass;
	    if ($just_parsed) { 
		die "Ooops" if ($sent != $sentence_count);
		$extracted .= "$sent ";
	    }
	    my $hypstr="";
	    while (/hyp={{{(.*?)}}}/g) { 
		$hypstr .= $1;
	    }
	    &debug("sent=$sent found nbest line: $_");
	    output_hyp($hypstr,$pass ? "(pass $pass)" : undef,$sent);
	} else {	 #if ($extracted or $recovered or $failed or $exception)
	    if (($rtfile && !$prev_over_rt)) {
		# unless: no rt file, or prev pass exists and is preferred
		output_hyp('',"no nbest",$sent);
		$prev_pass='';
	    } elsif ($prev_pass) {
	      prevpass:
		&debug('retrying',$prev_pass);
		++$nprev_pass;
		$_=$prev_pass;
		$pass=1;
		s/.*(NBEST sent=)/$1/;
		$prev_pass='';
		goto again;
	    } elsif (/hyp={{{(.*?)}}}/) { #recovered parse (no --rt-file)
		my $hyp='';
		$hyp .= " $1" while (/hyp={{{(.*?)}}}/g);
		superchomp(\$hyp);
		output_hyp($hyp,"recovered parse",$sent);
	    } else {
		my $error='';
		$error = " - $1" if /(ERROR: .*)/;
		output_hyp('',"failed to recover parse$error",$sent);
	    }
	}
    } elsif (/^NBEST MISSING/) {
	++$null_trans;
	&output_hyp("","marked 'NBEST MISSING'", $sent);
    }
}

sub summary {
    info(@_);
}

END {
    exit if &usage_called;
    my @have_lines = sort(grep { $hyps{$_} } keys %hyps);
    if ($reorder_lines) {
	info("Reordering lines into ascending order");
	my @expect_lines=keys %hyps;
	push @expect_lines,$sent if defined $sent && ! exists $hyps{$sent};
	&debug(@expect_lines);
	@expect_lines=@range_sents if scalar @range_sents;
	for (sort_num(@expect_lines)) {
	    &debug("printing sent=$_");
	    my $hyp=$hyps{$_};
	    info("sent=$_ hyp={{{$hyp}}}");
	    output_hyp('',"expected but never found in logs",$_) unless $hyp;
	    print "$hyp\n";
	}
    } else {
	if ($nhyp < $min_sents) {
	    my $pads=$min_sents-$nhyp;
	    print STDERR "Padding with $pads blank lines to reach --min-sents-output $min_sents.\n";
	}
	while ($nhyp < $min_sents) {
	    output_hyp('',undef,"decoder logs ended prematurely");
	}
    }
    write_file($found_log,join("\n",@have_lines)) if $found_log;

    info("Out of $N rescored best translations, the maximum original rank seen was $max_orig_rank, and the average was " . $sum_orig_rank/$N) if $N;
    info("SUMMARY:");
    while (my($dispo,$count)=each %dispositions) {
	$dispo = "OK" unless $dispo;
	info("$count sentences: $dispo");
    }
    print_number_summary();
    &restore_stdout if $outfile_name;
    &restore_stderr if $logfile_name;
}
