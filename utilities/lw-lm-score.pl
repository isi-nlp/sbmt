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
my $BLOBS;

BEGIN {
   $scriptdir = &File::Basename::dirname($0);
   ($scriptname) = &File::Basename::fileparse($0);
    $ENV{BLOBS}='/home/hpc-22/dmarcu/nlg/blobs' unless exists $ENV{BLOBS};
   $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
    $BLOBS=$ENV{BLOBS};
    my $libgraehl="$BLOBS/libgraehl/unstable";
    unshift @INC,$libgraehl if -d $libgraehl;
    my $decoder="$BLOBS/mini_decoder/unstable";
    unshift @INC,$decoder if -d $decoder;
    unshift @INC, $scriptdir;
}

require "libgraehl.pl";

### arguments ####################################################

my $output;
my $sent_out='/tmp/lwlm_sentences.XXXXXX';
my $lmat=1;
my $lm='';
my $lwlm="LangModel";
my $sri_ngram='ngram';
my $srilm='';
my $sri_debug=1;
my $raw;
my $rawsri;
my $plain_input;
my $score_only;
my $print_orig;
my $rm_sent_out;
my $hyp='hyp';
my $foreign_tree='foreign-tree';
my $tree='tree';
my $ftag='foreign-sentence';
my $fstart="<$ftag>";
my $fend="</$ftag>";
my $ratio=1;
my $unknown_word_penalty=20;
my $openclass;
my $ngram_order=3;
my $quiet;
my $strip_unk=1;
my $showmis;
my $lmcost_attr='lm1';
my $clmf='clm-';
my $clmlr;
my $clmopen=1;
my $clmverbose=1;
my $N=3; # clm order
my @options=(
             q{transform nbests or plain english text (one per line) into sentences ready for external LM scoring.  optionally perform external LM scoring and report differences vs. decoder lmcost}
             ,q{note: to convert an SRI lm:  LangModel -lm-in lm.SRI -lm-out lm.LW -trie2sa}
             ,q{note: to train an SRI lm: ngram-count -sort -text lm.training -lm lm.SRI}
             ,["attr-lm-cost=s"=>\$lmcost_attr,"(regex) lmcost attribute name"]
             ,["ngram-order=i"=>\$ngram_order,"(affects SRI lm) limit ngrams to this order (2=bigram etc)"]
             ,["plain-text-input!"=>\$plain_input,"don't expect nbests but rather score each input line"]
             ,["at-numclass!"=>\$lmat,"replace digits with '\@' before scoring"]
             ,["sentence-output=s"=>\$sent_out,"write lw LM input sentences here"]
             ,["rm-sentence-output!"=>\$rm_sent_out,"remove --sentence-output file when done"]
             ,["lm-ngram=s"=>\$lm,"read lw LM from here, and immediately run $lwlm -prob -lm [--lm-ngram] -in [--sentence-output]"]
             ,["sri-lm-ngram=s"=>\$srilm,"read SRI LM from here, and immediately run $sri_ngram -unk -ppl [--sentence-output] -debug 1"]
             ,["debug-level=i"=>\$sri_debug,"higher = more info from SRI toolkit.  10 shows all ngrams used"]
             ,["unknown-word-penalty=s"=>\$unknown_word_penalty,"cost (neglog10 prob) for an unknown word when closed class (not openclass) lm"]
             ,["open-class-lm!"=>\$openclass,"treat unknown words as <unk> rather than skipping and assessing --unknown-word-penalty.  note: if srilm is trained openclass (with -unk), this option cannot be effectively turned off!"]
             ,["only-scores!"=>\$score_only,"Just print (one per line) the LM scores corresponding to input"]
             ,["score-output|output=s"=>\$output,"Final (scored) output here"]
             ,["raw-lwlm-output=s"=>\$raw,"Raw output from LangModel copied here"]
             ,["raw-srilm-output=s"=>\$rawsri,"Raw output from ngram copied here"]
             ,["print-original!"=>\$print_orig,"Print original line following each scored line"]
             ,["show-ratio!"=>\$ratio,"Print the ratio of {lw,sri} lm cost to decoder lmcost"]
             ,["quiet!"=>\$quiet,"Show summary info only (not per-sentence scores/ratios)"]
             ,["strip-unknown!"=>\$strip_unk,'Remove @UNKNOWN@ from hyp (should, if you used unknown_word_rules with empty lmstring']
             ,["mismatch!"=>\$showmis,'Show nbest lines that have differing ngram scores'],
    ,["clm-lr=s"=>\$clmlr,"load clm 3grams from clm-left=x.left and clm-right=x.right (SRI lm format *only*)"]
             );

my $unk='@UNKNOWN@';
my ($usagep,@opts)=getoptions_usage(@options);
&show_cmdline;
show_opts(@opts);

if ($sri_debug<1) {
 $sri_debug=1;
 info_remember("minimum sri debug level is 1");
}

&argvz;

my $mdblob="$BLOBS/mini_decoder/unstable/";

my $haves=$lm||$srilm;
my ($S,$sfile) = &openz_out($sent_out) if $haves;

&openz_stdout($output) if $output;

my $capture_3brackets=&capture_3brackets_re;
my $capture_num='('.&number_re.')';

my @sent=();
my @lmcost=();
my @lines=();
my @flines=(); # don't need to save these
my @lr=qw(left right);
my @clmevents=();
while(<>) {
    next unless ($plain_input || /\b\Q$hyp\E=$capture_3brackets/o);
    push @lines,$_;
    my $line=$plain_input ? $_ : $1;
    my @E=split ' ',$line;
    if ($clmlr) {
        die unless /\b\Q$foreign_tree\E=$capture_3brackets/o;
        my $ftree=$1;
        my @F;
        while ($ftree =~ /(\S+)/g) {
            unless ($1 =~ /^\(/) {
                my $word=$1;
                $word =~ s/\)+$//;
                push @F,$word
            }
        }
        my $fshift=($F[0] eq $fstart) ? 0 : 1;
        unshift @F,$fstart if $fshift;
        push @F,$fend unless $F[$#F] eq $fend;
        push @flines,[@F];
        #&debug(@F,$ftree);
        die unless /\b\Q$tree\E=$capture_3brackets/o;
        my $etree=$1;
        my $l=[];
        my $r=[];
        my @r;
        #[fspan][espan]
        my $cl=$N-1;
#        debug(\@E,\@F,$etree);
        while ($etree =~ /\)\[(\d+),(\d+)\]\[(\d+),(\d+)\]/g) {
            my $el1=min($4,$3+$cl);
            my $fl=max(0,$1+$fshift-1);
            my $fr=min($#F,$2+$fshift); # because [a,b) means F[b] is outside to right
            my $wfl=$F[$fl];
            my $wfr=$F[$fr];
            my $er0=max($3,$4-$cl);
            my @evl=reverse($wfl,@E[$3..$el1-1]);
            my @evr=(@E[$er0..$4-1],$wfr);
#            debug("[$1,$2][$3,$4] => f$fl.e[$3,$el1) / e[$er0,$4).f$fr",@evl," / ",@evr);
            push @$l,[@evl];
            push @$r,[@evr];
        }
        pop @$l; # exclude TOP because clm scores do too
        pop @$r;
        # &debug($l,$r,$etree);
        push @clmevents,[$l,$r];
    }
    $line =~ s/\Q$unk\E//g if $strip_unk;
#    my $cost=0;
#    $cost=$1 if !$plain_input && /$lmcost_attr=$capture_num/;
    my $cost=$plain_input?0:getcost($lmcost_attr,$_);
    push @lmcost,$cost;
    superchomp(\$line);
    $line =~ s/\d/\@/g if $lmat;
    my $s="<s> $line </s>";
    log_numbers("(decoder) $lmcost_attr=$cost for sentence") unless $plain_input;
    print $S $s,"\n" if $haves;
    push @sent,$s;
}

close $S if $haves;

my $number=&number_re;

my @lw=();

if ($srilm || $clmlr) {
    $sri_ngram=which_prog($sri_ngram,"$mdblob$BLOBS/mini_decoder/unstable/$sri_ngram");
    &debug($sri_ngram);
}

sub get_sri_ngram
{
    my ($R,$W,@ng)=@_;
    my $N=scalar @ng;
    my $g=join ' ',@ng;
#    &debug($g,$R,$W);
    print $W "$g\n";
#    flush($W); # not needed because open2/open3 have autoflush
    while(<$R>) {
#        &debug("looking for $g",$_);
        if (/^\s*\Q$g\E\s*$/) {
            my $p;
            for (1..$N) {
                $p=<$R>;
#                &debug($_,$p);
            }
            die "couldn't parse p( ... ) = [ngram] 0.3425 [ log10(0.3425) ]: $p"
                unless $p =~ /^\s*p\(.*\)\s*=\s*\[(\d)gram\] (\S*) \[ (\S*) \]\s*$/;
            my ($ngram,$prob,$logprob)=($1,$2,$3);
            my $cost=-$logprob;
            &debug($g,$cost,"$ngram-gram");
            return $cost;
        }
    }
}
sub getcost
{
    my ($attr,$line)=@_;
    my $cost=0;
    $cost=$1 if $line=~/\Q$attr\E=$capture_num/;
    $cost;
}
my @myclm=([],[]); # don't really need unless we want to augment nbest output at end
if ($clmlr) {
    for my $r (0..1) {
        my $featname=$clmf.$lr[$r];
        my $lmfile="$clmlr.$lr[$r]";
        my @cmd=($sri_ngram,'-lm',$lmfile,'-ppl','-','-debug',2,'-order',$N,'-unk');
        &debug(@cmd);
        my ($R,$W)=&exec_pipe({'mode'=>($clmverbose?'std':'quiet')},@cmd);
        for my $i (0..$#clmevents) {
            my $ev=$clmevents[$i]->[$r];
            my $mycost=0;
            for my $g (@$ev) {
                my $cost=get_sri_ngram($R,$W,@$g);
                $mycost+=$cost;
            }
            my $ncost=getcost($featname,$lines[$i]);
            &debug($ncost," ($featname) vs. my ",$mycost,"diff=",$mycost-$ncost,"ratio=",$mycost/$ncost);
            check_mismatch($featname,$mycost,$ncost,$i);
            push @{$myclm[$r]},$mycost;
        }
        close $R;
        close $W;
        &cleanup;
    }
}
if ($lm) {
    require_files($sfile,$lm);
    $lwlm=which_prog($lwlm,"$mdblob/$lwlm");
    my @cmd=($lwlm,'-prob','-lm',$lm,'-in',$sfile,'-order',$ngram_order);
    my $cmdtext=join ' ',@cmd," 2>/dev/null";
    print STDERR "scoring w/ LWLM: $cmdtext\n";
    my @output=`$cmdtext`;
#    my @output=exec_filter($cmdtext);
    my $scored_text='';
    my $n=0;
    for (@output) {
        if (/^\(($number)\)$/) {
            my ($cost)=$1;
            $cost=~s/^-//;
            log_numbers("(LW LangModel) cost=$cost for sentence");
#            print $cost;
#            print "$scored_text\n";
#            print $lines[$n] if ($print_orig);
            push @lw,$cost;
            ++$n;
        } elsif (!$score_only) {
            chomp;
            $scored_text=" $_";
            chomp $scored_text;
        }
    }
    write_file_lines($raw,undef,undef,@output) if ($raw);
#    print STDERR @output;
}

my @sri=();

if ($srilm) {
    require_files($sfile,$srilm);
    my @cmd=($sri_ngram,'-lm',$srilm,'-ppl',$sfile,'-debug',$sri_debug,'-order',$ngram_order);
    push @cmd,'-unk' if $openclass;
    my $cmdtext=join ' ',@cmd;#," 2>/dev/null";
    print STDERR "scoring w/ SRI LM: $cmdtext\n";
    my @output=`$cmdtext`;
#    my @output=exec_filter($cmdtext);
    my $scored_text='';
    my $n=0;
    my $OOV;
    for (@output) {
        last if /^file \Q$sfile\E\: \d+ sentences\, \d+ words\, \d+ OOVs$/;
        if (/\, (\d+) OOVs$/) {
            $OOV=$1;
            log_numbers(($openclass ? "WARNING " : "")."(SRI ngram): $OOV OOV for sentence $n") if $OOV;
        } elsif (/^(\d+) zeroprobs,\s+logprob=\s*($number) ppl=/) {
            my ($zeroprobs,$cost)=($1,$2);
            log_numbers_gen("ERROR: (SRI ngram) $zeroprobs zeroprobs for sentence [$sent[$n]]") if $1;
            $cost=~s/^-//;
            $cost += $unknown_word_penalty * $OOV if !$openclass && $OOV;
            $OOV=undef;
            log_numbers("(SRI ngram) cost=$cost for sentence");
            push @sri,$cost;
#            print $cost;
            #            print "$scored_text\n";
            #            print $lines[$n] if ($print_orig);
            ++$n;
        } elsif (!$score_only) {
            chomp;
            if (/^\<s\>.*\<\/s\>$/) {
                warning_gen("mismatch between SRILM sent: [$_] and decoder sent [$n: $sent[$n]]") if ($_ ne $sent[$n]);
                $scored_text=" $_";
            }
        }
    }
    my @p=();
    for (@output) {
        if (s/p\( (\S+) \| (\S+) (...)?\)\s*= (\S+) (\S+) \[ -?(\S+) \] \/ 1?/\np($4$2!!$1)=/) {
            my $prob=$6;
            @p=map { $_+$prob } @p;
            unshift @p,$prob;
            #@p=($prob);
            $_.=join " ",@p;
        } else {
            @p=();
        }
        s/^(\<s\> )/\n$1/;
        print;
    }
    push @output,"\n";
    write_file_lines($rawsri,undef,undef,@output) if ($rawsri);
}

my $usesri=(scalar @sri >= scalar @sent); # extra total over all sentences to be
                                        # ignored

my $uselw=(scalar @lw == scalar @sent);

fatal("Didn't get a score for every sentence as expected from SRI LM $srilm") if ($srilm && !$usesri);
fatal("Didn't get a score for every sentence as expected from LW LM $lm") if ($lm && !$uselw);

sub printq {
print(@_) unless $quiet;
}

sub ratio_near_1 {
    my ($r,$tol)=@_;
    $tol=.001 unless defined $tol;
    return $r<1+$tol && $r>1-$tol;
}

sub check_mismatch {
    my ($type,$score,$refscore,$i,$ratio,$print)=@_;
    printq(" ${type}=$score") if $print;
    my $l=$refscore;
    return unless $l;
    my $r=$score/$l;
    printq(" ${type}ratio=$r") if $ratio;
    my $d=abs($r-1);
    my $eps=.0001;
    log_numbers("$type/decoder ratio=$r");
#    &debug($d<=$eps?"ok":"mismatch",$score,"should be=",$l);
    if ($d>$eps) {
        log_numbers("$type mismatch ratio=$r error=$d nbest=$i");
        return unless $showmis;
        my $line=$lines[$i];
        chomp $line;
        print "$line $type=$score $type-ratio=$r $type-error=$d\n";
    }

}
sub show_mismatch {
    my ($type,$score,$i)=@_;
    my $l=$lmcost[$i];
    return unless $l;
    check_mismatch($type,$score,$l,$i,$ratio,1);
}

for (0..$#sent) {
    print $lines[$_] if ($print_orig);
    printq("lmcost=$lmcost[$_]") if $lmcost[$_];
    if ($uselw) {
        show_mismatch('lw',$lw[$_],$_);
    }
    if ($usesri) {
        show_mismatch('sri',$sri[$_],$_);
    }
    if ($clmlr) {
        for my $r (0..1) {
            my $featname=$clmf.$lr[$r];
            check_mismatch($featname,$myclm[$r]->[$_],getcost($featname,$lines[$_]),$_,$ratio,1);
        }
    }
    if (!$score_only) {
      printq(" $sent[$_]");
   }
    printq("\n");
}

flush();

&all_summary;

unlink $sfile if $haves&&$rm_sent_out;
