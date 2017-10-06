#!/usr/bin/perl -w
#echihabi

use strict;
use Getopt::Long "GetOptions";
use File::Temp qw/ :mktemp  /;
use Cwd 'abs_path';
use File::Basename;

my $scriptdir = dirname(abs_path($0));
my $DelNonLatin = "$scriptdir/delete-nonlatin.sh";
my $DeTok = "$scriptdir/lw_detokenize.pl";
my $BleuTool ="$scriptdir/scoreTranslation";
my $TmpDir ="/tmp";
my @References = ();
my $ID;
my $Help;
my $ref;
my $NSize = 4;
my $Metric = "bleuNistVersion";
my $NistTok = 1;
my $Detok = 1;
my $LC = 1;
my $Format;
my $XML;
&GetOptions('i=s' => \$XML,
	    'help' => \$Help,
	    'n=i' => \$NSize,
	    'metric=s' => \$Metric,
	    'nisttok=i' => \$NistTok,
	    'detok=i' => \$Detok,
	    'if=s' => \$Format,
	    'lc=i' => \$LC
	    );

foreach $ref (@ARGV){
    push @References,$ref;
}

die("USAGE $0 -i <xml file to score> <ref1> <ref2>... -if <input format: xline|xml> [-n <bleu-gram:4*> -metric <bleu|bleuNistVersion*> -nisttok <0|1*> -detok<0|1*> -lc<0|1*>]") if (defined($Help) || !defined($XML) || $#References==-1 || !defined($Format));

die("Wrong Input Format $Format. Should be xml or xline.\n") if ($Format !~ /^(xml|xline)$/); 


my $Dir = mkdtemp("$TmpDir/xml_bleu_XXXXXX");

if ($Format eq 'xml'){
    &extract_1best($XML,"$Dir/hyp.1best");
}
else{
    &process_1best($XML,"$Dir/hyp.1best");
}

#delete non latin words and detokenize 
if ($Detok){
    `cat $Dir/hyp.1best | $DelNonLatin | $DeTok > $Dir/hyp.1best.detok`;
}
else{
    `cat $Dir/hyp.1best | $DelNonLatin > $Dir/hyp.1best.detok`;
}

#normalize
&normalize_file("$Dir/hyp.1best.detok","$Dir/hyp.1best.detok.norm");



my $i=1;
foreach $ref (@References){
    &normalize_file($ref,"$Dir/ref.$i");
    $i++;
}

#score
my $cmd = "$BleuTool -$Metric -hyp $Dir/hyp.1best.detok.norm -n $NSize ";
$i=1;
foreach $ref (@References){
    $cmd .="$Dir/ref.$i ";
    $i++;
}

print "$XML\t";

my $result = `$cmd`;

$result =~ s/\/.*hyp\.\d+//;

print $result;

#clean
`rm -rf $Dir`;


sub extract_1best {
    my ($in,$out) = @_;
    open(IN,"$in") or die("Cannot open $in for reading!\n");
    open(OUT,">$out") or die("Cannot open $out for writing!\n");
    my $line;
    my $print = 0;
    my ($bestScore,$bestHyp,$best);
    while($line=<IN>){
	if ($best && $line =~ /<words>\s*(.*)<\/words>/){
	    $bestHyp = $1;
	    $best=0;
	}
	elsif($line =~ /<\/seg>/){
	    print OUT &unescape_xml($bestHyp)."\n";
	}
	elsif($line =~ /<hyp score=\"(\S+)\"/){
	    if (!defined($bestScore) || ($1 > $bestScore)){
		$bestScore=$1;
		$best = 1;
	    }
	}
	elsif($line =~ /<seg segid=/){
	    undef $bestScore;
	    undef $bestHyp;
	    $best=0;
	}
    }
    close(IN);
    close(OUT);
}

sub process_1best {
    my ($in,$out) = @_;
    open(IN,"$in") or die("Cannot open $in for reading!\n");
    open(OUT,">$out") or die("Cannot open $out for writing!\n");
    my $line;
    while($line=<IN>){
	print OUT  &unescape_xml($line);
    }
    close(IN);
    close(OUT);
}

sub unescape_xml {
    my $s = shift @_;
    $s=~ s/\&lt\;/\</g;
    $s=~ s/\&gt\;/\>/g;
    $s=~ s/\&apos\;/\'/g;
    $s=~ s/\&quot\;/\"/g;
    $s=~ s/\&amp\;/\&/g;
    $s =~ s/UNKNOWN_(\S+)/$1/g;
    return $s;
}

sub normalize_file{
    my ($in,$out) = @_;
    my $line;
    open(IN,"$in") or die("Cannot open $in for reading!");
    open(OUT,">$out") or die("Cannot open $out for reading!");
    while($line=<IN>){
	print OUT &normalize_text($line) ."\n";
    }
    close(IN);
    close(OUT);
}




#from mteval-v11.pl
sub normalize_text {
    my ($norm_text) = @_;
    if ($NistTok){
# language-independent part:
	$norm_text =~ s/<skipped>//g; # strip "skipped" tags
	$norm_text =~ s/-\n//g; # strip end-of-line hyphenation and join lines
	$norm_text =~ s/\n/ /g; # join lines
	$norm_text =~ s/(\d)\s+(?=\d)/$1/g; #join digits
	$norm_text =~ s/&quot;/"/g;  # convert SGML tag for quote to "
	$norm_text =~ s/&amp;/&/g;   # convert SGML tag for ampersand to &
	$norm_text =~ s/&lt;/</g;    # convert SGML tag for less-than to >
	$norm_text =~ s/&gt;/>/g;    # convert SGML tag for greater-than to <
	
# language-dependent part (assuming Western languages):
	$norm_text = " $norm_text ";
#    $norm_text =~ tr/[A-Z]/[a-z]/ unless $preserve_case;
	$norm_text =~ s/([\{-\~\[-\` -\&\(-\+\:-\@\/])/ $1 /g;   # tokenize punctuation
	$norm_text =~ s/([^0-9])([\.,])/$1 $2 /g; # tokenize period and comma unless preceded by a digit
	$norm_text =~ s/([\.,])([^0-9])/ $1 $2/g; # tokenize period and comma unless followed by a digit
	$norm_text =~ s/([0-9])(-)/$1 $2 /g; # tokenize dash when preceded by a digit
	$norm_text =~ s/\s+/ /g; # one space only between words
	$norm_text =~ s/^\s+//;  # no leading space
	$norm_text =~ s/\s+$//;  # no trailing space
    }
    return lc ($norm_text) if ($LC);
    return $norm_text;
}
