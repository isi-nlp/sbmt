#!/usr/bin/perl -w

if (scalar(@ARGV) != 1) { die "Need training directory\n"; }
$dir = shift;

@files = ("e","f","e.info","f.info","e.vcb","f.vcb","f_e.cooc","e_f.cooc","GIZA.combined.apl");

system("hostname");
system("date");

warn "Creating working files (".localtime().")\n"; 
for $file (@files) {
    $outFile = $file; if ($file =~ /cooc/) { $outFile .= ".whole"; }
    if (not -e "$dir/$file") {
	if (-e "$dir/$file.bz2") { system ("cat $dir/$file.bz2 | bunzip2 > $outFile"); }
	else { die "Can't find $dir/$file\n"; }
    }
    else { system ("ln -s -f $dir/$file ./$outFile"); }
    if (not -e "$outFile") { die "Couldn't create $outFile\n"; }
}
    
warn "Extracting lexicon (".localtime().")\n";
if (not -e "GIZA.combined.apl") { die "Can't find GIZA.combined.apl\n"; }
system ( "( lex_extract.out f e  GIZA.combined.apl -infofile1 f.info -infofile2 e.info -aplflag 1 -corporaweights CorporaWeights.list > LEXICON ) >& log.lex_extract") == 0 or die "Can't extract lexicon!\n";

$cutoff = 0.0001;


warn "Reading file LEXICON (".localtime().")\n";
open (TEXT, "LEXICON") or die "LEXICON: $!";
while ($line = <TEXT>) {
    ($prob, $f, $e) = split (" ",$line);
    if (not exists $eStrToInt{$e}) { 
	warn "New e word $e\n"; next;
    }
    if (not exists $fStrToInt{$f}) { 
	warn "New f word $f\n"; next;
    }
    $Lex{$f}{$e} = $prob;
    $TotalF{$f} += $prob;
    $TotalE{$e} += $prob;
}
close(TEXT);

my $lex_file_normal = "normal.t4.final";
my $lex_file_invers = "invers.t4.final";

warn "Writing output\n";
open (OUT_NORMAL, ">$lex_file_normal") or die "$lex_file_normal: $!";
open (OUT_INVERS, ">$lex_file_invers") or die "$lex_file_invers: $!";
for $f (sort keys %Lex) {
    for $e (sort keys %{$Lex{$f}}) {
	$prob = (1.0*$Lex{$f}{$e})/$TotalF{$f};
	if ($prob >= $cutoff) {
	    print OUT_NORMAL "$fStrToInt{$f} $eStrToInt{$e} $prob\n";
	    $GoodLex{$fStrToInt{$f}}{$eStrToInt{$e}} = 1;
	}
	$prob = (1.0*$Lex{$f}{$e})/$TotalE{$e};
	if ($prob >= $cutoff) {
	    print OUT_INVERS "$eStrToInt{$e} $fStrToInt{$f} $prob\n";
	    $GoodLex{$fStrToInt{$f}}{$eStrToInt{$e}} = 1;
	}
    }
}
close(OUT_NORMAL);
close(OUT_INVERS);
%Lex = ();

warn "Reducing cooc files\n";
open (OUT, ">e_f.cooc") or die "e_f.cooc: $!";
open (TEXT, "e_f.cooc.whole") or die "e_f.cooc.whole: $!";
while ($line = <TEXT>) {
    @line = split " ",$line;
    if (exists $GoodLex{$line[1]}{$line[0]}) { print OUT $line; }
}
close(TEXT); close(OUT);
open (OUT, ">f_e.cooc") or die "f_e.cooc: $!";
open (TEXT, "f_e.cooc.whole") or die "f_e.cooc.whole: $!";
while ($line = <TEXT>) {
    @line = split " ",$line;
    if (exists $GoodLex{$line[0]}{$line[1]}) { print OUT $line; }
}
close(TEXT); close(OUT);

warn "Done\n";

# LEXICON contains counts of links
# this scripts transforms them into probabilities
# it produces both P(f|e) and P(e|f)
# also produces "vocabulary files"

# "normal" means: 
# - format is "f e prob"
# - for a given 'f', the probabilities add up to 1

# "invers" means: 
# - format is "e f prob"
# - for a given 'e', the probabilities add up to 1
