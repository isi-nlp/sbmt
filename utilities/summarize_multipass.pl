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
    $ENV{BLOBS}='/auto/nlg-01/blobs' unless exists $ENV{BLOBS};
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
my $csv_output;

my $skip="";
#1,5,10,14,17,24,30,38,39";
my $login='';

my %name=qw(
            hyp hyp
            NBEST NBEST
            sent sent
            nbest nbest
           );

my @options=(
             q{given nbests on input, pull out one sent=N and call it sent=1},
             ,["output=s"=>\$output,"output here"]
             ,["skip=s"=>\$skip,"skip e.g. 1-2,5,6,10-100"]
             ,["log-input=s"=>\$login,"decoder log"]
             ,["csv-output=s"=>\$csv_output,"CSV output"]
             );

my $unk='@UNKNOWN@';
my ($usagep,@opts)=getoptions_usage(@options);
&show_cmdline;
show_opts(@opts);
#my @skips=parse_range($skip);
#my %skip=%{multiset_from_list_ref(\@skips)};
my %skip=multiset_from_list(parse_range($skip));

&argvz;

&openz_stdout($output) if $output;
my $LOG=&openz($login);

my $capture_3brackets=&capture_3brackets_re;
my $capture_natural='(\d+)';
my $capture_num='('.&number_re.')';
my $capture_fname='\b([^ =]+)=';

my @score_sent_pass=();

my $nsent=0;
my @fields=qw(time edges 1best_score nbests_score num_1bests num_nbests options);
my %field=index_from_list(@fields);
&debug(\%field);

my @table;

sub add {
    my ($pass,$name,$val)=@_;
    my $i=$field{$name};
    defined $i or die "no $name field";
    $table[$pass]->[$i]+=$val;
}


sub log_time {
    my ($pass,$time,$edges)=@_;
    my $pre="pass=$pass ";
    log_numbers("time=$time edges=$edges",$pre);
    add($pass,"time",$time);
    add($pass,"edges",$edges);
}

sub log_score {
    my($sent,$pass,$nbest,$score)=@_;
    add($pass,"num_nbests",1);
    add($pass,"nbests_score",$score);
    my $pre="pass=$pass ";
    log_numbers("score=$score",$pre);
    if ($nbest==0) {
        add($pass,"num_1bests",1);
        add($pass,"1best_score",$score);
        log_numbers("score=$score","1best $pre");
        $score_sent_pass[$sent][$pass-1]=$score;
    }
}


my ($sent,$pass,$status);
while(<$LOG>) {
    if (/pass=(\d+) options: (.*)/) {
        $table[$1]->[$field{options}]=$2;
        info($_);
    }
    
    if (/^decode-finish id=$capture_natural pass=$capture_natural status=(\S+)/o) {
        ($sent,$pass,$status)=($1,$2,$3);
        if ($status eq 'fail') {
            warning_gen("sent=$sent [pass=$pass] failed. skipping");
            $skip{$sent}=1;
        }
    }
#    next if defined($sent) && exists $skip{$sent};
    
    if (/time=\[$capture_num wall sec, $capture_num user sec.* - $capture_natural edges and $capture_natural equivalences created/) {
        my ($wall,$user,$edges,$equivs)=($1,$2,$3,$4);
        $sent='?' unless defined $sent;
        $pass='?' unless defined $pass;
        log_time($pass,$user,$edges);
    }
}


while(<>) {
    
    if (/^\Q$name{NBEST}\E\s+\Q$name{sent}\E=$capture_natural\s+/o) {
        my $sent=$1;
        next if exists $skip{$sent};
        /\b\Q$name{nbest}\E=$capture_natural\b/o or die;
        my $nbest=$1;
        /\btotalcost=$capture_num\b/o or die;
        my $score=$1;
        /\bpass=$capture_natural\b/o or die;
        my $pass=$1;
        log_score($sent,$pass,$nbest,$score);
    }
}

flush();

#debug_force(@table);

if ($csv_output) {
    my $o=openz_out($csv_output);
    print $o "pass,",(join ',',@fields),"\n";
    my $i=0;
    for (0..$#table) {
        my $r=$table[$_];
        next unless defined $r;
        print $o $_,",",(join ',',@$r),"\n";
    }
    print $o "\n";
    my $headed;
    for (0..$#score_sent_pass) {
        my $l=$score_sent_pass[$_];
        next unless ref $l;
        unless ($headed) {
            $headed=1;
            print $o "sent/pass,",(join ',',(1..scalar @$l)),"\n";
        }
        print $o "$_,",(join ',',@$l),"\n";
    }
}


&all_summary;
