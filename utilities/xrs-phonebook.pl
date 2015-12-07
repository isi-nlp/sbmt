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

my $nonce='iyhuai8756839874lnhfiuoasdnvwoyikriyuikdnv48323ndls'; # for easy search/rewrite/replace

my $ids;
my $textids;
my $outxrs;
my ($etrees,$fstrings,$align);
my $ghkm;
my $htmldir;
my $maxperid;
my $sortsmall;
my $restrictre='derivation={{{[^}]*}}}';
my $viz;
$viz="$scriptdir/viz-tree-string-pair.pl";
$viz='/home/rcf-78/sdeneefe/bin/viz-tree-string-pair.pl' unless -x $viz;
my $lang='ara';
my $dirmax=500;
my $sentdirmax=500;
my $quietmiss;
my $training_html;
my $justpdf=1;
my $modn=100;
my $rebuildsents;

my @options=(
q{creates a concordance of (the N shortest) training examples responsible for each rule id mentioned in the input. open <-htmldir>/index.html to browse.  xrs rules according to the provided ids are taken as STDIN or file arguments.  NOTE: when running latex, copy David Chiang's pst-qtree.aux to the current working dir for prettier output.},
 ["dir-max=s"=>\$dirmax,"Limit html subdirectory size to this many items"],
 ["ruledir-mod-n=s"=>\$modn,"distribute rules in directory ruleA where A = ruleid mod N.  N=0 distributes by -dir-max instead"],
# ["sentdir-size"=>\$sentdir,"Limit training subdirectory size to this"],
 ["viz-script=s"=>\$viz,"Steve's tree/align/string .tex creator"],
 ["language=s"=>\$lang,"foreign string is (eng,ara,chi) for latex viz"],
 ["ids=s"=>\$ids,"scan for ids from this file"],
 ["text-ids=s"=>\$textids,"(not a filename) additional rule ids"],
 ["restrict-ids-re=s",=>\$restrictre,"look for rule ids only the part of id lines within this (PERL) regexp"],
 ["out-xrs=s"=>\$outxrs,"write (feature-less) xrs rules to this file, suitable for reuse as xrs input"],
 ["etrees=s"=>\$etrees,"english trees one per line"],
 ["fstrings=s"=>\$fstrings,"foreign strings one per line"],
 ["align=s"=>\$align,"alignments one per line"],
 ["ghkm-derivs=s"=>\$ghkm,"ghkm derivations (space separated multiple files ok) with per-sentence rule ids"],
 ["htmldir=s"=>\$htmldir,"write index.html here"],
 ["max-per-id=s"=>\$maxperid,"include no more than N training examples per rule id"],
 ["smallest-first!"=>\$sortsmall,"sort, keeping the max-per-id *shortest* training examples"],
 ["quiet-missing!"=>\$quietmiss,"don't note individual missing xrs rules"],
 ["just-pdf!"=>,\$justpdf,"do not create any html file for training sentences linking to pdf/tex/ps, just link to pdf"],
 ["rebuild-sents-from=s"=>,\$rebuildsents,"file containing sentence numbers to generate sentN/sentence-number.tex,.html - skips all the steps up until then"],
);

&show_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
show_opts(@opts);

$sentdirmax=$dirmax;

&argvz;

### write out html files
use CGI qw(:standard);

sub my_start_html {
    start_html(-head=>meta({-http_equiv=>'Content-Type',
                            -content=>'text/html; charset=UTF-8'}),-title=>@_);
}

sub my_end_html {
    end_html."\n"
}


my %lookfor_id;
my %lookfor_text;
my %rule_for_id;

my $idre=qr/-?[0-9]+/;
my $numre=qr/[0-9]+/;
my $lineno=0;

sub mkdir_base {
    &mkdir_force(&mydirname);
}


my $n_id_url=0;
my $c_id_url=0;

sub rule_subdir_n {
    my ($id)=@_;
    my $g=\$lookfor_id{$id};
    return (abs($id) % $modn) if $modn;
    if (!defined $$g) {
        ++$c_id_url if ($n_id_url % $dirmax == 0);
        $$g=$c_id_url;
        ++$n_id_url;
    }
    return $$g;
}

sub url_for_id {
    my($id)=@_;
    my $N=rule_subdir_n($id);
    "rule$N/$id.html";
}

sub file_for_id {
    my $p="$htmldir/".&url_for_id;
    mkdir_base($p);
    $p;
}

my $n_sent_url=0;
my $c_sent_url=0;
my %sent_loc;
sub url_for_sent {
    my ($sent,$ext,$rel)=@_;
    my $g=\$sent_loc{$sent};
    if (!defined $$g) {
        ++$c_sent_url if ($n_sent_url % $dirmax == 0);
        $$g=$c_sent_url;
        ++$n_sent_url;
    }
    $ext=($justpdf ? ".pdf" : ".html") unless defined $ext;
    my $r="$sent$ext";
    $rel ? $r : "sent$$g/$r";
}

sub file_for_sent {
    my $p="$htmldir/".&url_for_sent;
    mkdir_base($p);
    $p;
}

sub parse_id_line {
    ++$lineno;
    my ($l)=@_;
    &debug("parse ids",$l);
    $l=escapeHTML($l);
    my $l_restrict;
    if ($restrictre) {
        $l=~s/($restrictre)/$nonce/o;
        $l_restrict=$l;
        $l=$1;
    }
    &debug("idre",$idre,$l);
    $l=~s{($idre)}{
                '<a href="'.url_for_id($1).'">'.escapeHTML($1).'</a>'
            }goe if defined $l;
    if ($restrictre) {
        $l_restrict=~s/\Q$nonce\E/$l/;
        $l=$l_restrict;
    }
    print INDEX "\n<li>$l\n" if ($htmldir);
}

my %corpus_lines;
my %just_lines;

sub write_rule_html {
    my ($id)=@_;
    my $out=file_for_id($id);
    my $O=openz_out($out);
    #header(-charset=>'UTF8'),
    my $title="Rule $id";
    print $O my_start_html($title),h1($title);
    if (exists $rule_for_id{$id}) {        
        print $O p,escapeHTML($rule_for_id{$id}),hr;
        if (exists $corpus_lines{$id}) {
            my @out=map {
                ' <a href="../'.url_for_sent($_).'">'.escapeHTML($_).'</a>'
                }
            @{$corpus_lines{$id}};
            
            my $ONLY=$maxperid?" (showing only the ".($sortsmall?"smallest":"first")." $maxperid)":"";
#            if ($maxperid && scalar @out > $maxperid) {                pop @out;            }
            print $O "Used in training sentences$ONLY:",p,@out;
#            ul(li(\@out));
        } else {
            print $O "Not found in training corpus.";
        }
    } else {
        print $O p,"Rule text not found for id=$id";
    }
    print $O &my_end_html;
    close $O;
}


unless ($rebuildsents) {
my $htmlfile="$htmldir/index.html" if $htmldir;
if ($htmldir) {
    mkdir_force($htmldir);
    open INDEX,">",$htmlfile or die;
    print INDEX my_start_html("Provenance of xrs rule ids from training"),"\n<ol>\n";
}

if ($ids) {
    my $fh=openz($ids);
    while(<$fh>) {
        parse_id_line($_);
    }
}

parse_id_line($textids) if ($textids);

if ($htmldir) {
    print INDEX "\n</ol>\n",&my_end_html;
    close INDEX;
}

my $n_ids=scalar keys %lookfor_id;

info_remember("$n_ids rule ids under scrutiny\n");
info("Scanning xrs rules: ");
my $nr=0;
while(<>) {
    ++$nr;
    show_progress($nr);
    if (/^(.*) \#\#\# .*\bid=($idre)\b/o) {
        my ($text,$id)=($1,$2);
        if (exists $lookfor_id{$id}) {      
            &debug("found rule id=",$id,$text);
            if (exists $rule_for_id{$id} && $rule_for_id{$id} ne $text) {
                warning_gen("rule id=[$id] defined multiple times with different text: [$text] vs. older [$rule_for_id{$id}]");
            }
             #FIXME: diff id same rule text problem?
            if (exists $lookfor_text{$text} && $lookfor_text{$text} != $id) {
                warning_gen("same rule text for rules id=[$id] vs. older id=[$lookfor_text{$text}]");
            }
            $lookfor_text{$text}=$id;
            $rule_for_id{$id}=$text; 
        }
    }
}
done_progress($nr);

my $n_found=scalar keys %rule_for_id;
info_remember("found the xrs rule text for $n_found out of $n_ids rule ids under scrutiny\n");

my $OUTXRS;

if ($outxrs) {
    $OUTXRS=openz_out($outxrs);
}

for (sort keys %lookfor_id) {
    if (exists $rule_for_id{$_}) {
        print $OUTXRS "$rule_for_id{$_} ### id=$_\n" if $outxrs;
    } else {
        warning_gen("text not found for rule id=[$_] (unknown-word,glue,TOP,green?)") unless $quietmiss;
    }
}

if ($htmldir) {
    my $bad="$htmlfile.with.bad.links.html";
    rename $htmlfile,$bad;
    open IN,"<",$bad;
    open OUT,">",$htmlfile;
    while(<IN>) {
        s{(<a href=\"[^"]+\">)($idre)(</a>)}{
                      exists $rule_for_id{$2} ? "$1$2$3" : $2
                  }goe;
                      print OUT $_;
    }
}
exit unless $ghkm;

close ARGV;
@ARGV=split ' ',$ghkm;


my %train_line;

info("Scanning GHKM output: ");
my $ng=0;
while(<>) {
    ++$ng;
    show_progress($ng);
    if (/^(.*) \#\#\# .*\blineNumber=($numre)\b/o && exists $lookfor_text{$1}) {
        my ($text,$line)=($1,$2);
        my $id=$lookfor_text{$text};
        &debug("found rule id=$id text on line $line",$text);
        #push @{$corpus_lines{$id}},$line;
        &push_no_dup(\@{$corpus_lines{$id}},$line);
        $train_line{$line}=1;
    }
}
done_progress($ng);

### sort/uniq and prune corpus lines

if ($sortsmall) {
    info("Retrieving training corpus sentence lengths from file $etrees: ");
    die "FIXME:don't know lengths of sentences for sorting" unless $etrees;
    my $L=openz($etrees);
    my $n=0;
    while(<$L>) {
        ++$n;
        show_progress($n);
        $train_line{$n}=length $_ if exists $train_line{$n};
    }
    done_progress($n);
    close $L;
}


for my $p (values %corpus_lines) {
    if ($sortsmall) {
        @$p = sort { $train_line{$a} <=> $train_line{$b} } @$p;
    }
    if ($maxperid && scalar @$p > $maxperid) {
        splice @$p,$maxperid;
    }
    @$p=sort {$a<=>$b} @$p unless $sortsmall;
    $just_lines{$_}=1 for @$p;
}

if ($htmldir) {
    for (keys %lookfor_id) {
        write_rule_html($_);
    }
}

}
# end unless rebuildsents


### write out training data files

if ($rebuildsents) {
   my $f=openz($rebuildsents);
   while(<$f>) {
      while (m#sent(\d+)/(\d+)\b#g) {
         my ($dirno,$id)=($1,$2);
$just_lines{$id}=1;
$sent_loc{$id}=$dirno;
      }
   }
}

my $tmp="$htmldir/tmp";

sub produce_tex {
    my ($sent,$e,$f,$a)=@_;
    my $tex=file_for_sent($sent,".tex");
#    my $ps=file_for_sent($sent,".ps");
    my ($ef,$ff,$af)=(to_tempfile($e,"$tmp.e"),to_tempfile($f,"$tmp.f"),to_tempfile($a,"$tmp.a"));
    my $cmd="$viz -l $lang -e $ef -f $ff -a1 $af -o $tex -t";
#    print STDERR $cmd,"\n";
    system $cmd;
    unlink $ef;
    unlink $ff;
    unlink $af;
}

sub write_file_html {
    return if $justpdf;
    my ($sent,$e,$f,$a)=@_;
    &debug("write_file_html",$sent,$e,$f,$a);
    my $out=file_for_sent($sent);
    my $O=openz_out($out);
    my $title="Training sentence $sent";
    print $O my_start_html($title),h1($title);
    print $O h3(a({'href'=>url_for_sent($sent,".tex",1)},"TeX").' '.a({'href'=>url_for_sent($sent,".ps",1)},"PostScript"). ' '.a({'href'=>url_for_sent($sent,".pdf",1)},"PDF"))
        ,h3("etree:"),escapeHTML($e),h3("foreign:"),escapeHTML($f),h3("alignment:"),escapeHTML($a);
    print $O &my_end_html;
    close $O;
}

die("ERROR: supply etree/f/align files when producing html output") if $htmldir && (!$align || !$fstrings || !$etrees);
my $A=openz($align);
my $F=openz($fstrings);
my $E=openz($etrees);

my $n=0;
my $n_training_lines=scalar keys %just_lines;
info_remember("Processing the $n_training_lines relevant training sentences: ");
for my $c (sort {$a<=>$b} keys %just_lines) {
    my ($e,$f,$a);
    while ($n<$c) {
        $e=<$E>;
        $f=<$F>;
        $a=<$A>;
        show_progress($n);
        ++$n;
    }
    # assert defined $a $f $e i.e. corpus file has requested lines in it.
    die "no e for line $c" unless defined($e);
    die "no f for line $c" unless defined($f);
    die "no a for line $c" unless defined($a);
    write_file_html($c,$e,$f,$a);
    produce_tex($c,$e,$f,$a);
}
done_progress($n);


&all_summary;
