#!/usr/bin/env perl
#
# Author: graehl

#NOTE: the only reason sbtm-score/cost is treated separately is because if get
# score (as opposed to cost) output form the decoder, the feature name changes

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


my $show_agreed_details=0;
my $debug_sums=0;
my $mdblob="$BLOBS/mini_decoder/unstable/";
my $sbtmfeat='sbtm-cost';

### arguments ####################################################

my $get_final_weights=1;
my $nbest_out='';
my $rules_out='';
my $log_out;
my $gview="grammar_view";
my $grammar_archive;
my $brf_grammar;
my $total_score_name="sbtm-score";
my $sbtm_cost_name="sbtm-(?:cost|score)";
my $weight_string;
my $quiet=0;
my $weight_file;
my $feat_sep=" ### ";
my $wrong_out;
my $corrected_out;
my $precision=10;
my $epsilon=1e-4;
my $nbest_rule_sum;
my $sent_rules_out;
#my $corpus_map;

my @blacklist=qw(id nbest sent failed-parse hyp etree tree foreign foreign-tree derivation failed-parse used-rules totalcost lmcost lm-cost  align s t pass lm lm-unk lm-open-unk); # sbtm-cost
my @extra_features=qw(unk-rule glue-rule text-length derivation-size);
my %blacklist;
$blacklist{$_}=1 for (@blacklist);

my @options=(
             q{read decoder nbests and write rules used and/or recomputed feature vectors (warning of mismatches)}
             ,["nbest-out=s"=>\$nbest_out,"Write here: nbests with feature vectors replaced with features from rules"]
             ,["wrong-out=s"=>\$wrong_out,"Write here: only corrected nbests, attr=val left alone but with corrected-attr=corrected-val being added"]
             ,["nbest-rule-sum!"=>\$nbest_rule_sum,"Show (for wrong-out/corrected-out only) attr-rulesum={{{}}} with list of id:cost from rule"]
             ,["corrected-out=s"=>\$corrected_out,"Write here: only corrected nbests, attr=val renamed to wrong-attr=val"]
             ,["rules-out=s"=>\$rules_out,"Write here: rule usages (rules will be listed only the first time they occur)"]
             ,["per-sentence-rules-out=s"=>\$sent_rules_out,"replacing {} with the sentence number, create per-sentence xrs rule files. e.g. xrs.{}.gz"]
#             ,["corpus-map=s"=>\$corpus_map,"Use corpus-map file (Jens format) for remapping sentence numbers (both for nbest output and per-sentence-rules-out"]
             ,["log-out=s"=>\$log_out,"log saved to here (also to stderr)"]
             ,["grammar-archive=s"=>\$grammar_archive,"archived translation rules grammar"]
             ,["brf-grammar=s"=>\$brf_grammar,"text brf rules (alternative to grammar-archive)"]
             ,["weight-string=s"=>\$weight_string,"string with feature and lm exponents (weights) like: a:-1,b:2.5"]
             ,["weight-file=s"=>\$weight_file,"file with first line: a weight-string: feature and lm exponents (weights), single line like: a:-1,b:2.5"]
             ,["quiet!"=>\$quiet,"Discard stderr of $gview"]
             ,["epsilon=f"=>\$epsilon,"relative difference in costs allowed to consider rules/nbest features equal"]
             ,["agreeing-sums!"=>\$show_agreed_details,"Show the rule sums even for features that agree w/ nbest"]
             ,["precision=i"=>\$precision,,"significant digits to print"]
             ,["final-weights!"=>\$get_final_weights,"get back from grammar_view all the actual weights used (to verify dot product on nbest)"]
            );

my $final_weights_to='/tmp/sbtm-score.final.weights.XXXXXX';

my %used_rule_ids;

sub first_time_id {
    my ($id)=@_;
    if (not exists $used_rule_ids{$id}) {
        $used_rule_ids{$id}=1;
        return 1;
    }
}

my ($usagep,@opts)=getoptions_usage(@options);
tee_stderr($log_out) if $log_out;
&show_cmdline;
show_opts(@opts);

set_default_precision($precision);
set_epsilon($epsilon);
&argvz;


$gview=which_prog($gview,"$mdblob/$gview");

my @base_gv=($gview);
push @base_gv,'--quiet','1';
push @base_gv,'--all-output','-0';
push @base_gv,'--output','-';
push @base_gv,'--weight-string',$weight_string if $weight_string;
push @base_gv,'--weight-file',$weight_file if $weight_file;


my ($N,$nfile)=&openz_out($nbest_out) if $nbest_out;
my ($R,$rfile)=&openz_out($rules_out) if $rules_out;
my ($W,$wfile)=&openz_out($wrong_out) if $wrong_out;
my ($C,$cfile)=&openz_out($corrected_out) if $corrected_out;

my ($final_weights)=$weight_file ? read_file_lines($weight_file) : '';
chomp $final_weights;
$final_weights = $weight_string if $weight_string;
sub finalize_weights {
    my @cmd=@base_gv;
    push @cmd,'--id-input','-0';
    push @cmd,'--final-weights-to','-';
    my $c=join ' ',@cmd;
    my $fw=`$c`;
    chomp $fw;
    info_remember("final weights from grammar_view: $fw");
    $final_weights=$fw if $fw;
}
&finalize_weights if $get_final_weights;

my $gotrules;
my ($Rrule,$Wid);

END {
close $Rrule if $Rrule;
close $Wid if $Wid;
&cleanup;
}

if ($brf_grammar or $grammar_archive) {
    $gotrules=1;
    my @cmd=@base_gv;
    push @cmd,'--id-input','-';
    push @cmd,'--grammar-archive',$grammar_archive if $grammar_archive;
    push @cmd,'--brf-grammar',$brf_grammar if $brf_grammar;
#use File::Temp qw/mktemp/;
#    if ($get_final_weights) {
#        $final_weights_to=mktemp($final_weights_to) if $final_weights_to =~ /XXXX$/;
#        push @cmd,'--final-weights-to',$final_weights_to;
#    }

    info("starting: ".(join ' ',@cmd));
    ($Rrule,$Wid)=exec_pipe({'mode'=>($quiet?'quiet':'std')},@cmd);
#    if ($get_final_weights) {
#        my $gv_final_weights=get_rule("w");
####        sleep 1; $final_weights=read_file_line($final_weights_to);# ugly hack: should pass in FD or grab first line of input?
#        chomp $gv_final_weights;
#        if ($gv_final_weights) {
#            info("got grammar_view final weights: $gv_final_weights");
#            $final_weights=$gv_final_weights;
#        }
#    }
} elsif ($rules_out) {
    fatal("no grammar supplied, so can't write used rules to $rules_out");
}



my %weights=parse_comma_colon_hash($final_weights);
info("Using final weights:");
write_hash(undef,\%weights);
#while ($final_weights =~ /\b([^:,]+)(:[^:,]*)?/g) {
#my $fname=$1;
my %weights_id=();
for my $fname (keys %weights) {
    my $fid=word2id($fname);
    $weights_id{$fid}=$weights{$fname};
    info_remember("Expecting feature $fname from weights");
}
word2id($_) for (@extra_features);
# prepopulate so that we later override these nbest features

#&openz_stdout($nbest_out) if $nbest_out;


sub get_rule {
    my ($id)=@_;
    die unless $gotrules;
    print $Wid "$id\n";
    flush($Wid);
    my $rule=<$Rrule>;
    chomp $rule;
    return $rule;
}


my $capture_3brackets=&capture_3brackets_re;
my $capture_num='('.&number_re.')';
my $capture_cost='(?:(10|e)\^)?'.$capture_num;
my $capture_fname='\b([^ =]+)=';
# use after succesful $capture_cost
sub get_cost {
    my ($base,$n)=@_;
    if ($base) {
        return 0 unless $n;
        if ($base eq '10') {
            return -1*$n;
        } elsif ($base eq 'e') {
            return -1*ln_to_log10($n);
        }
    } else {
        return $n;
    }
}

my $any_mismatch;

sub mismatch {
    my ($f,$v1,$v2,$n1,$n2,$index,$addl)=@_;
    $n1='nbest' unless $n1;
    $n2='rules' unless $n2;
    $addl=" - individual rule occurences: $addl" if $addl;
    $addl='' unless $addl;
    warning("missing $n1") unless (defined $v1);
    warning("missing $n2") unless (defined $v2);
    return 2 unless defined($v1) && defined($v2);
    if (!relative_epsilon_equal($v1,$v2)) {
        log_numbers("ERROR: $index has wrong $f: $n1=$v1 $n2=$v2 change=".($v2-$v1)." relative-change=".relative_change($v1,$v2).$addl);
        count_info("ERROR: ($n1,$n2) mismatch: $f");
        $any_mismatch=1;
 #       log_numbers("ERROR: mismatch on $f: $n1=$v1 $n2=$v2 change=".($v2-$v1)." relative-change=".relative_change($v1,$v2)." individual rule occurences: $addl");
#        count_info("ERROR: (rules,nbest) mismatch: $f");
        return 1;
    } else {
        count_info("(rules,nbest) agreed: $_[0]");
        log_numbers("agreed on $f: $n1=$v1 $n2=$v2" .($show_agreed_details?$addl:''));
        return 0;
    }
}

my @raw_feats_byid=();

sub raw_sum_id {
    my ($fid)=@_;
    return join ' ',@{at_default(\@raw_feats_byid,$fid)};
}

sub raw_sum {
    my ($fname)=@_;
    return raw_sum_id(word2id($fname));
}

sub feat_rule_sums {
    my ($fname)=@_;
    if ($nbest_rule_sum) {
        return "$fname-rulesum={{{".raw_sum($fname)."}}}";
    } else {
       return '';
   }
}

while (<>) {
    $any_mismatch=0;
    my $savedline=$_;
    my $nbest=$savedline;
    #    chomp $nbest;
    next unless (/NBEST /);
    /\bsent=([^ ]+)/;
    my $sentid=$1;
    /\bnbest=(\d+)/;
    my $nbestid=$1;
    my $index="sent=$sentid #$nbestid";
    next unless (/\bderivation=$capture_3brackets/o);
    my $derivation=$1;
    $derivation=~s/\[\d+,\d+\]//g; # remove span marking if --show-span
    my $sbtm_cost=0;
    my @featsum_byid=();
    my @derivfeat_byid=();
    my $sum_rule_sbtm_cost=0;
    my @deriv_nodes=();
    @raw_feats_byid=();
    while ($derivation =~ /(-?\d+)/g) {
        my $id=$1;
        #        no warnings;
        #        ++$used_rule_ids{$1};
        &count_info_gen("id=[$id]");
        push @deriv_nodes,$id;
    }
    s/\bsbtm-score=/$sbtmfeat=/g;
    if ($gotrules) {
        for my $id (@deriv_nodes) {
            my $rule=get_rule($id);
            warning_gen("Rule id=[$id] not in grammar") if (!$rule);
            if ($rules_out && first_time_id($id)) {
                print $R "$rule\n";
                count_info("printed rule.");
            }
            if ($rule =~ /\Q$feat_sep\E/og) {
                $rule =~ s/$capture_fname$capture_3brackets//g;
                while ($rule =~ /$capture_fname($capture_cost)/og) {
                    my $name=$1;
                    $name=$sbtmfeat if $name eq 'sbtm-score';
                    next if exists $blacklist{$name};
#                    next if $name eq $total_score_name;
                    my $raw_cost=$2;
                    my $cost=get_cost($3,$4);
                    my $fid=word2id($name);
                    log_numbers("(single rule)$name=$cost");
                    no warnings;
                    info("$name fid=$fid sum=$featsum_byid[$fid]") if $debug_sums;
                    $featsum_byid[$fid]+=$cost;
#                    $raw_feats_byid[$fid].=" $id:$raw_cost";
                    push @{at_default(\@raw_feats_byid,$fid)},"$id:$cost";
                    info("$name fid=$fid sum=$featsum_byid[$fid] added $cost from rule $rule") if $debug_sums;
                }
            }
        }
    }
    my %repl=();
    my @nbest_byid=();
    while ($nbest =~ /$capture_fname$capture_cost/og) {
        my $name=$1;
        next if exists $blacklist{$name};
        my $cost=get_cost($2,$3);
        log_numbers("(nbest)$name=$cost");
        if (haveword($name)) {  # was seen on rules or in weights
            if ($gotrules) {
                my $fid=word2id($name);
                $nbest_byid[$fid]=$cost;
                my $rulesumcost=$featsum_byid[$fid];
                $rulesumcost=0 unless $rulesumcost;
                my $ruleraw=raw_sum_id($fid);
                log_numbers("(rule-sum)$name=$cost");
                $repl{$name}=$rulesumcost if mismatch($name,$cost,$rulesumcost,'nbest','rules',$index,$ruleraw);
            }
        } else {
            count_info("unexpected feature on nbests: $name");
        }
    }
    if ($get_final_weights) {
        my $fid=word2id($sbtmfeat);
        my $dotprod=dot_product_array_hash(\@featsum_byid,\%weights_id);
        mismatch($sbtmfeat,$nbest_byid[$fid],$dotprod,'nbest','nbest_features*weights',$index)
    }
    if ($any_mismatch) {
        count_info("ERROR: nbest line with features not agreeing with sum over rules (same weights/grammar?)");
        if ($corrected_out) {
            my $corrected=$nbest;
            $corrected =~ s/\b([^ =]*)=($capture_cost)/exists $repl{$1}?"wrong-$1=$2 $1=$repl{$1}".feat_rule_sums($1):"$1=$2"/ge;
            print $C $corrected if $corrected ne $nbest;
        }
        if ($wrong_out) {
            my $wrong=$nbest;
            $wrong =~ s/\b([^ =]*)=($capture_cost)/exists $repl{$1}?"$1=$2 corrected-$1=$repl{$1}".feat_rule_sums($1):"$1=$2"/ge;
            print $W $wrong if $wrong ne $nbest;
        }
    }
    $nbest =~ s/\b([^ =]*)=($capture_cost)/"$1=".(exists $repl{$1}?$repl{$1}:$2)/ge;
    count_info("overwrote from rule sums ".scalar(keys %repl)." features on an nbest derivation");
    print $N $nbest if $N;
    #    my $feats=$1;
}

#flush();

&all_summary;



#&cleanup;
