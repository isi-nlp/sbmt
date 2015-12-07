#!/usr/bin/env perl
use Getopt::Long;
use POSIX;
use strict;

sub usage {
    print STDERR "\n";
    print STDERR "help -- print this message\n";
    print STDERR "d    -- driver file (format:  [latex anno |] line no)\n";
    print STDERR "a    -- latex anno file (will override driver anno)\n";
    print STDERR "e    -- etree file\n";
    print STDERR "f    -- fstring file\n";
    print STDERR "a1   -- first alignment file\n";
    print STDERR "a2   -- second alignment file\n";
    print STDERR "o    -- out file\n";
    print STDERR "l    -- language of fstring (default: eng, other options: chi, ara)\n";
    print STDERR "t    -- tight tree format\n";
    print STDERR "debug -- turn on debug messages\n";
    exit;
}
my $DEBUG=0;

sub set_debug { $DEBUG=1; }

sub debug_print {
    print STDERR @_ if $DEBUG;
}
my $jarpath=".";
my $driverfile;
my $annofile;
my $etreefile;
my $fstringfile;
my $alfile1;
my $alfile2;
my $outfile;
my $lang;
my $tight;

GetOptions(  "help"       => \&usage
	   , "jarpath:s"  => \$jarpath
	   , "d=s"  => \$driverfile
	   , "a=s"  => \$annofile
	   , "e=s"    => \$etreefile
           , "f=s"  => \$fstringfile
           , "a1=s"    => \$alfile1
           , "a2=s"    => \$alfile2
           , "o=s"      => \$outfile
	   , "l=s"  => \$lang
	   , "t",   => \$tight
           , "debug"      => \&set_debug
	     );

my $preword;
my $postword;
if ($lang) {
    debug_print("language specified = $lang\n");
    if ($lang eq "chi") {
	$preword = "\\begin{CJK}{GB}{song}";
	$postword = "\\end{CJK}";
    } elsif ($lang eq "ara") {
	$preword = "\\RL{";
	$postword = "}";
    } elsif ($lang ne "eng") {
	warn "Unknown language '$lang', defaulting to english\n";
    }
} else {
    debug_print("language specified = none (default to english)\n");
}

sub readlines($\@) {
    my $fn = shift;
    my $slinesref = shift;
    my %out;

    my @sorted_slines = sort { $a <=> $b } @{$slinesref};

    open F, $fn or die "Can't open $fn: $!\n";
    my $next = shift @sorted_slines;
    #print STDERR "next = $next\n";
  OUTER:
    while (<F>) {
	while ( $. == $next ) { 
	    $out{$.} = $_;
	    $next = shift @sorted_slines || last OUTER;
	    #print STDERR "next = $next\n";
	}
    }
    close F;
    warn "# there is stuff remaining!" if @sorted_slines;

    my @out;
    for my $i ( @{$slinesref} ) {
	push(@out, $out{$i} );
	#print STDERR "outline $i\n";
    }

    return @out;
}

die "$outfile should end in .tex" unless ($outfile =~ /\.tex$/);
my $outdir = `dirname $outfile`;
chomp $outdir;
my $outbase = $outfile;
$outbase =~ s/\.tex$//;
debug_print "Base is $outbase\n";
my $dvifile = "$outbase.dvi";
my $psfile = "$outbase.ps";

my @sourcelines;
my @desc;
my @estrings;
if ($driverfile) {
    open D, $driverfile or die "Can't open $driverfile: $!\n";
    while (<D>) {
	if (/^\s*((.*)\s+\|\s+)?\s*(\d+)\s*$/) {
	    #print STDERR "desc = $2, sl = $3\n";
	    push @desc, $2;  # optional -- will be overriden by annofile
            push @sourcelines, int($3);
	}
    }

    @estrings = readlines($etreefile, @sourcelines) or die "Can't open $etreefile: $!\n";
} else {
    open E, $etreefile or die "Can't open $etreefile: $!\n";
    @estrings = <E>;
    close E;
}

map { 
    chomp;
    # strip tildes and numbers
    s/~\d+~\d+ [-\d\.]+//g;
    # make latex safe
    s/([\$_%\{\}&])/\\$1/g;
    s/\^/\\\^\{\}/g;
    # dot every nonterm
    s/(\(\s*)(\S)/$1\.$2/g;
    # bold any triple prestarred nonterm -- for highlighting
    s/(\(\s*)\.\*\*\*(\S+)/$1\.\\textbf{$2}/g;
    # replace parens with squares
    s/\(/ \[ /g;
    s/\)/ \] /g; 
} @estrings;
# wrap terminals with indices
my $lineid=0;
foreach my $string (@estrings) {
    my $wordid=0;
    while ($string =~ / ([^\s\}\{]+) \]/) {
	my $word = $1;
	my $match = " $word ]";
	# make match perl safe
	$match =~ s/([\\\|\(\)\[\{\^\$\*\+\?\.])/\\$1/g;
	# bold any triple prestarred words -- for highlighting
	$word =~ s/\*\*\*(\S+)/\\textbf{$1}/;
	my $repl = " \\rnode{s$lineid"."e$wordid}{$word} ]";
	$string =~ s/$match/$repl/;
	$wordid++;
	debug_print "String to $string\n";
    }
    $lineid++;
}

my @fstrings;
if ($driverfile) {
    @fstrings = readlines($fstringfile, @sourcelines) or die "Can't open $fstringfile: $!\n";
} else {
    open F, $fstringfile or die "Can't open $fstringfile: $!\n";
    @fstrings = <F>;
    close F;
}

if ($annofile) {
    if ($driverfile) {
	@desc = readlines($annofile, @sourcelines) or die "Can't open $annofile: $!\n";
    } else {
	open F, $annofile or die "Can't open $annofile: $!\n";
	@desc = <F>;
	close F;
    }
}

$lineid=0;
# put labels on f
my @modfs = ();
foreach my $string (@fstrings) {
    my $wordid=0;
    my @newf = ();
    foreach my $word (split /\s+/, $string) {
	# make latex safe
	$word =~ s/([\$_%\{\}&])/\\$1/g;
	# bold any triple prestarred things -- for highlighting
	while ($word =~ s/^\*\*\*(\S+)$/\\textbf{$1}/g) {};
	push @newf, "\\rnode{s$lineid"."f$wordid}{$preword$word$postword}";
	$wordid++;
    }
    push @modfs, join ' \hspace{0.3cm}', @newf;
    $lineid++;
}

my @aligns1;
if ($driverfile) {
    @aligns1 = readlines($alfile1, @sourcelines) or die "Can't open $alfile1: $!\n";
} else {
    open A1, $alfile1 or die "Can't open $alfile1: $!\n";
    @aligns1 = <A1>;
    close A1;
}
my @aligns2;
if ($alfile2) {
    if ($driverfile) {
	@aligns2 = readlines($alfile2, @sourcelines) or die "Can't open $alfile2: $!\n";
    } else {
	open A2, $alfile2 or die "Can't open $alfile2: $!\n";
	@aligns2 = <A2>;
	close A2;
    }
}

my @printals = ();
# calculate the alignments and make the commands
for (my $i = 0; $i < @aligns1; $i++) {
    my @set = ();
    my $al1 = $aligns1[$i];
    my (%a1s, %a2s);
    map {/(\d+)-(\d+)/; $a1s{$1}{$2} = 1 } split /\s+/, $al1;
    if ($alfile2) {
	my $al2 = $aligns2[$i];
	map {/(\d+)-(\d+)/; $a2s{$1}{$2} = 1 } split /\s+/, $al2;
    }
    # find all arcs in both alignments and in al1 only
    foreach my $e (keys %a1s) {
	foreach my $f (keys %{$a1s{$e}}) {
	    my $item="{s$i"."e$e}{s$i"."f$f}";
	    if ($alfile2) {
 		if (exists $a2s{$e}{$f}) {
		    $item = "\\ncline[linewidth=.02mm]{-}$item";
		}
		else {
		    $item="\\ncline[linewidth=.02mm,linestyle=dashed]{-}$item";
		}
	    }
	    else {
		$item = "\\ncline[linewidth=.02mm]{-}$item";
	    }
	    push @set, $item;
	}
    }
    # find all arcs in al2 only
    if ($alfile2) {
	foreach my $e (keys %a2s) {
	    foreach my $f (keys %{$a2s{$e}}) {
		next if (exists $a1s{$e}{$f});
		my $item="\\nczigzag[fillstyle=none,linewidth=.02mm,coilwidth=1mm,linearc=.01]{o-o}{s$i"."e$e}{s$i"."f$f}";	    
		push @set, $item;
	    }
	}
    }
    push @printals, join "\n", @set;
}
    
debug_print "English 1 is:\n";
debug_print "$estrings[0]\n";
debug_print "Foreign 1 is:\n";
debug_print "$modfs[0]\n";
debug_print "Align 1 is:\n";
debug_print "$printals[0]\n";


# preparatory stuff
if ($lang eq "chi") {
    open OUT, ">$outfile.utf8" or die "Can't open $outfile.utf8: $!\n";
} else {
    # don't need to write utf-8 for english (hopefully)
    open OUT, ">$outfile" or die "Can't open $outfile: $!\n";
}

my $prefix;
while ( <DATA> ) {
    ($prefix,$_) = split /:/, $_, 2;
    next unless length($prefix) == 3;
    print OUT if ( $prefix eq 'all' || $prefix eq $lang);
}

if ($tight) {
  print OUT "\\psset{levelsep=36pt,nodesep=2pt,treesep=24pt,treefit=tight}\n";
} else {
  #print OUT "\\psset{nodesep=0.1cm}\n";
  print OUT "\\psset{levelsep=36pt,nodesep=2pt,treesep=24pt,treefit=loose}\n";
}

print OUT "\n\\begin{CJK}{GB}{song}\\end{CJK}\n" if $lang eq "chi";

for (my $i = 0; $i < @estrings; $i++) {
    my $tree = $estrings[$i];
    my $fstring = $modfs[$i];
    my $als = $printals[$i];
    my $sl = $sourcelines[$i];
    my $desc = $desc[$i];
    print OUT "\\newpage\n\n" if $i > 0;
    print OUT "Training Data line = $sl\n\n" if $sl;
    print OUT "$desc\n\n" if $desc;
    print OUT "\\begin{tabular}{c}\n";
    print OUT "\\shrinkboxtokeepaspect(9.5in,5.5in){\\Tree $tree} \\\\ \n\n";
    print OUT "\\vspace{1in} \\\\ \n\n";
    print OUT "\\shrinkboxtokeepaspect(9.5in,1in){$fstring}\n";
    print OUT "\\end{tabular}\n";
    print OUT "$als\n";
}

print OUT "\\end{document}\n";

close OUT;
# convert utf-8 to something LaTex can use
if ($lang eq "chi") {
    `/home/nlg-01/contrib/local/j2sdk1.4.2_03/bin/java -jar /home/rcf-78/sdeneefe/bin/java/encoding_converter.jar $outfile.utf8 - UTF-8 GB2312 > $outfile`;
}

__DATA__
all:\documentclass[letterpaper]{article}
all:\usepackage[left=12mm,right=12mm,top=20mm,bottom=12mm,landscape,dvips]{geometry}
all:\usepackage{times,tabularx,supertabular}
all:\usepackage{qtree,pst-tree,pstricks,pst-node,pst-coil}
all:\include{pst-qtree}
chi:\usepackage{CJK}
ara:\usepackage{arabtex,atrans,nashbf,utf8}
ara:\usepackage{times}
all:\makeatletter
all:\def\shrinkboxtokeepaspect(#1,#2)#3{\shrinkboxto(#1,0){\shrinkboxto(0,#2){#3}}}
all:\def\shrinkboxto(#1,#2){\pst@makebox{\@shrinkboxto(#1,#2)}}
all:\def\@shrinkboxto(#1,#2){%
all:\begingroup
all:\pssetlength\pst@dima{#1}%
all:\pssetlength\pst@dimb{#2}%
all:\ifdim\pst@dima=\z@\else
all:\pst@divide{\pst@dima}{\wd\pst@hbox}\pst@tempc
all:\ifdim\pst@tempc pt > 1 pt
all:\def\pst@tempc{1 }%
all:\fi
all:\edef\pst@tempc{\pst@tempc\space}%
all:\fi
all:\ifdim\pst@dimb=\z@
all:\ifdim\pst@dima=\z@
all:\@pstrickserr{%
all:\string\shrinkboxto\space dimensions cannot both be zero}\@ehpa
all:\def\pst@tempa{}%
all:\def\pst@tempc{1 }%
all:\def\pst@tempd{1 }%
all:\else
all:\let\pst@tempd\pst@tempc
all:\fi
all:\else
all:\pst@dimc=\ht\pst@hbox
all:\advance\pst@dimc\dp\pst@hbox
all:\pst@divide{\pst@dimb}{\pst@dimc}\pst@tempd
all:\ifdim\pst@tempd pt > 1 pt
all:\def\pst@tempd{1 }%
all:\fi
all:\edef\pst@tempd{\pst@tempd\space}%
all:\ifdim\pst@dima=\z@ \let\pst@tempc\pst@tempd \fi
all:\fi
all:\edef\pst@tempa{\pst@tempc \pst@tempd scale }%
all:\@@scalebox
all:\endgroup}
all:\pslongbox{Shrinkboxto}{\shrinkboxto}
all:
all:\DeclareRobustCommand*\textsubscript[1]{%
all:{\m@th\ensuremath{_{\mbox{\fontsize\sf@size\z@\selectfont#1}}}}}
all:\makeatother
all:
all:\pagestyle{myheadings}
all:\newcommand{\sentence}[1]{%
all:  \section*{#1}%
all:  \markboth{{#1}\hfill {$heading}\hfill}{{#1}\hfill {$heading}\hfill}}
all:
all:\begin{document}
all:\qtreecenterfalse
ara:
ara:\setcode{utf8}
ara:\setarab
