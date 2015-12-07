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


my ($foreign_start,$intro_f,$separator,$translated,$untranslated)=('<foreign-sentence>',1,'','-','-');
my ($plaintext,$parens,$show_comments,$foreign);
my $enc='bytes';

my %name=qw(
            seg seg
            NEMATCH NEMATCH
            FEATURES FEATURES
            FORCEUSAGE FORCEUSAGE
            english english
           );

my @options=(
q{Separates XML marked-up foreign input like: <seg> <NEMATCH english="a | ..." FEATURES=" BYLINE | ..." FORCEUSAGE=" 1 | ...">translated foreign words</NEMATCH> untranslated words" into two files, both one line per seg.  NOTE: assumes <seg>foreign sentence and markup</seg> are on THE SAME LINE"},
q{ TODO: are | inside english escaped somehow?},
q{(if -t and -u are the same, -t lines are printed first, alternating with -u).  If consumed-to is the same, the order is CTUCTU...},
["translated-to=s"=>\$translated,"write bylines here"],
["untranslated-to=s"=>\$untranslated,"write foreign text following bylines here"],
["consumed-to=s"=>\$foreign,"write the foreign text consumed by the --translated-to translation here"],

["separator=s"=>\$separator,"when a byline is found, append this separator to it (unless byline already ends in it)"],
["parens!"=>\$parens,"if line starts with matching UTF8 parens ( <BYLINE> ), then split the byline including the parens"],
["introduce-foreign-start=i"=>\$intro_f,"prepend a --foreign-start-token to the beginning of untranslated foreign (1 means do, 0 means don't)"],
["foreign-start-token=s"=>\$foreign_start,"used for --introduce-foreign-start"],
["plain-text-input!"=>\$plaintext,"input is one per line space-separated foreign words, not XML"],
["show-comments!"=>\$show_comments,"print to STDERR any lines beginning with #"],
["encoding=s"=>\$enc,"use this character encoding for all input and output"]
);


my ($usagep,@opts)=getoptions_usage(@options);
&show_cmdline;
show_opts(@opts);

&argvz;

#my $enc=':bytes';
$enc=&set_ioenc($enc);
use open IO => $enc;
my $T=&openz_out($translated);
my $U=&openz_out($untranslated);
my $F=$foreign ? &openz_out($foreign) : undef;
if ($enc) {
    binmode $T,$enc;
    binmode $U,$enc;
    binmode $F,$enc if $F;
}

my %xml=&xml_regexps;

sub print_foreign {
    my ($untr,$translated,$consumed)=@_;
    superchomp(\$untr);
    superchomp(\$translated);
    superchomp(\$consumed);
    if ($F) {
        print $F "$consumed\n";
        flush($F);
    }
    print $T "$translated\n";
    flush($T);
    print $U "$foreign_start " if $intro_f;
    print $U "$untr";
    print $U " </foreign-sentence>" if $intro_f;
    print $U "\n";
    flush($U);
}

my $openparen= $parens ? q{(\(?)\s*} : '()';

my $N=0;
while(<>) {
    print STDERR "\n$_\n" if ($show_comments && m|^\#|);
    flush(\*STDOUT);
    &superchomp(\$_);
    if ($plaintext) {
        ++$N;
        print_foreign($_,'','');
    } elsif (m|^<\Q$name{seg}\E>(.*)</\Q$name{seg}\E>$|) {
        ++$N;
        my $seg=$1;
        my $consumed='';
        my $translated='';
#        &debug($seg);
#        my $tagname;
        &debug("xml opentag:",$xml{opentag});
#        my $remove_parens_from_foreign;
        if ($seg =~ /^\s*$openparen($xml{opentag})/g) {
            my $has_open=$1;
            my ($tagname,$wholeopen)=($4,$2);
            &debug("has_open",$has_open);
            if ($tagname eq $name{NEMATCH}) {
                &debug("NEMATCH found",$wholeopen);
                my $english='';
                my $feature="";
                my $force='';
                while ($wholeopen =~/$xml{attr}/g) {
                    my ($name,$val)=($1,xml_attr_unquote($2));
                    $force=$1 if ($name eq $name{FORCEUSAGE} && $val =~ /^\s*(\w+)/);
                    $english=$1 if ($name eq $name{english} && $val =~ /^([^|]*)/);
                    $feature=$1 if ($name eq $name{FEATURES} && $val =~ /^([^| ]*)/);
                    &debug("xml attribute",$name,$val);
                }
                &info_remember("Found initial $name{NEMATCH} with $name{FEATURES}=$feature and $name{FORCEUSAGE}=$force");
                if ($seg =~ /($xml{tag})/g) {
                    my $wholeclose=$1;
                    my $closetagname=$3;
                    if ($closetagname eq $name{NEMATCH}) {
                        if ($force && $english && ($feature eq "BYLINE")) {
                            my $cont_at=$+[0];
                            my $has_close=($seg =~ m{\G\s*\)\s+}g);
                            &debug("has_close=",$has_close);
                            if ($parens && $has_open) {
                                if ($has_close) {
                                    $cont_at=$+[0];
                                    $english="( $english )";
                                    &debug("paren byline:",$english);
#                                    $remove_parens_from_foreign=true;
                                } else {
                                    goto foreign;
                                }
                            }
                            superchomp(\$english);
                            $english .= " $separator" if ($separator && $english !~ /(^| )\Q$separator\E$/);
#                            print $T $english;
#                            flush($T);
                            $translated=$english;
                            ## USING BYLINE:
                            if ($F) {
                                my $f=substr($seg,0,$cont_at);
                                $consumed=get_xml_free_text($f);
                            }
                            $seg=substr($seg,$cont_at);
                        }
                    } else {
                        warning_gen("initial $name{NEMATCH} was not immediately closed (saw [$wholeclose] first) - not using any byline for $name{seg} #[$N]");
                    }
                }
            }
        }
          foreign:
        my $untr=get_xml_free_text($seg);
#        $untr =~ s/^\s*\(\s*\)\s*// if ($remove_parens_from_foreign);
        print_foreign($untr,$translated,$consumed);
    }
}
my $type=$plaintext ? "UTF8 one-per-line":"xml <seg>";
info("DONE: $N input foreign sentences ($type) were split into (translated) $translated and (untranslated) $untranslated");
&info_summary;
