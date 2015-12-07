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
my $weight_string;
my $quiet=0;
my $weight_file;
my $feat_sep=" ### ";
my $wrong_out;
my $corrected_out;
my $precision=10;
my $epsilon=1e-5;
my $nbest_rule_sum;
my $sent_rules_out;
my $field="totalcost";

my @blacklist=qw(id nbest sent failed-parse hyp etree tree foreign foreign-tree derivation failed-parse used-rules lmcost lm-cost); # sbtm-cost
my @extra_features=qw(unk-rule glue-rule text-length derivation-size);
my %blacklist;
$blacklist{$_}=1 for (@blacklist);

my @options=(
             q{read decoder nbests and weights, complaining if the totalscore (should be dotprod feat*weights) is wrong}
             ,["log-out=s"=>\$log_out,"log saved to here (also to stderr)"]
             ,["weight-string=s"=>\$weight_string,"string with feature and lm exponents (weights) like: a:-1,b:2.5"]
             ,["weight-file=s"=>\$weight_file,"file with first line: a weight-string: feature and lm exponents (weights), single line like: a:-1,b:2.5"]
             ,["quiet!"=>\$quiet,"Discard stderr of $gview"]
             ,["epsilon=f"=>\$epsilon,"relative difference in costs allowed to consider rules/nbest features equal"]
             ,["agreeing-sums!"=>\$show_agreed_details,"Show the rule sums even for features that agree w/ nbest"]
             ,["precision=i"=>\$precision,"significant digits to print"]
             ,["fieldname=s",\$field,"check this field for totalcost"]
            );

my %used_rule_ids;


my ($usagep,@opts)=getoptions_usage(@options);
tee_stderr($log_out) if $log_out;
&show_cmdline;
show_opts(@opts);

set_default_precision($precision);
set_epsilon($epsilon);
&argvz;


my ($final_weights)=$weight_file ? read_file_lines($weight_file) : '';
chomp $final_weights;
$final_weights = $weight_string if $weight_string;

my %weights=parse_comma_colon_hash($final_weights);
my %seen_feats;
unless ($quiet) {
 info("Using final weights:");
 write_hash(undef,\%weights);
}
#while ($final_weights =~ /\b([^:,]+)(:[^:,]*)?/g) {
#my $fname=$1;
my %weights_id=();
for my $fname (keys %weights) {
    my $fid=word2id($fname);
    $weights_id{$fid}=$weights{$fname};
}
word2id($_) for (@extra_features);

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
    my ($f,$v1,$v2,$n1,$n2,$index)=@_;
    $n1='nbest' unless $n1;
    $n2='rules' unless $n2;
    warning("missing $n1") unless (defined $v1);
    warning("missing $n2") unless (defined $v2);
    return 2 unless defined($v1) && defined($v2);
    if (!relative_epsilon_equal($v1,$v2)) {
        log_numbers("ERROR: $index has wrong $f: $n1=$v1 $n2=$v2 change=".($v2-$v1)." relative-change=".relative_change($v1,$v2));
        count_info("ERROR: ($n1,$n2) mismatch: $f");
        $any_mismatch=1;
        return 1;
    } else {
        count_info("(rules,nbest) agreed: $_[0]") unless $quiet;
        log_numbers("agreed on $f: $n1=$v1 $n2=$v2");
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

    my $parsed_total;
    my $total=0;
    while ($nbest =~ /$capture_fname$capture_cost/og) {
        my $name=$1;
        my $cost=get_cost($2,$3);
        next if exists $blacklist{$name};
        log_numbers("(nbest)$name=$cost") unless $quiet;
        if ($name eq $field) {
            $parsed_total=$cost;
            next;
        }
        $seen_feats{$name}=1;
        if (exists $weights{$name}) {
            $total+=$weights{$name}*$cost;
        }
    }
    mismatch($field,$parsed_total,$total,$field,'weights*features',$index);
    if ($any_mismatch) {
        count_info("ERROR: nbest line with weighted feature sum != decoder totalcost");
    }
}

sub hash_difference {
  my ($h1,$h2,$n1,$n2)=@_;
  for (keys %$h1) {
      info_remember_gen("In $n1 but not $n2: [$_]") unless exists $h2->{$_};
  }
}

sub hash_difference_sym {
  my ($h1,$h2,$n1,$n2)=@_;
  hash_difference($h1,$h2,$n1,$n2);
  hash_difference($h2,$h1,$n2,$n1);
}

hash_difference_sym(\%seen_feats,\%weights,"nbest features","weights") unless $quiet;
&all_summary;

