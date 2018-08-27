#!/usr/bin/env perl 

#Copyright (c) 2005, Language Weaver Inc, All Rights Reserved.
#
#This software is provided 'as is' with no explicit or implied warranties.
#
#LW Tokenizer
$|++;
use strict;
use Getopt::Long "GetOptions";
use Benchmark;

my $_Conf;
my $_Debug=0;
my @RegExpBreak = ();
my @LocalRegExpReplace = ();
my @GlobalRegExpReplace = ();
my @GlobalRegExpSearch = ();

&GetOptions('conf=s' => \$_Conf,
	    'debug' => \$_Debug
	    );

if (!defined($_Conf)){
    die("USAGE: $0 -conf <configuration file> [-debug] < <input file>\n");
}

&load_conf;

my $st = new Benchmark;

my $line;
while($line=<stdin>){
    my $start = 0;
    my $out;
    my $idx;
    foreach my $r (0 .. $#GlobalRegExpReplace){
	if ($line =~ /$GlobalRegExpSearch[$r]/){
	    eval "\$line =~ $GlobalRegExpReplace[$r]";
	}
    }
    my @chars = split(//,$line);
    while(($idx = &get_next_word(\@chars,\$out,$start)) != -1 ){
	print STDERR "Working on <$out>:\n" if ($_Debug);
	my $processed=0;
	foreach my $regexp (@RegExpBreak){
	    if ($out =~ /$regexp/gi){
		my $strBreak = $&;
		my $strBreakLen = length($strBreak);
		my $p = pos($out) - $strBreakLen;
		print STDERR "\tMatched at $p on <$&> using $regexp\n" if ($_Debug);
		if ($p > 0){
		    print &get_token(\@chars,$idx,$idx+$p-1) ." ";
		    print "\t\tprebreak: ".&get_token(\@chars,$idx,$idx+$p-1)."\n" if ($_Debug);
		}
		print &get_token(\@chars,$idx+$p,$idx+$p+$strBreakLen-1) . " "; 
		print "\t\tbreak: ".&get_token(\@chars,$idx+$p,$idx+$p+$strBreakLen-1)."\n" if ($_Debug);
		$processed=1;
		$start = $idx+$p+$strBreakLen;
		last;
	    }
	}
	if (!$processed){
	    $out = &replace($out);
	    print $out." ";
	    $start = $idx + length($out);
	}
    }
    print "\n";
}

my $et = new Benchmark;

print STDERR "done in ".timestr(timediff($et , $st))."\n";

##get token between starting and ending offsets
sub get_token {
    my ($chars,$s,$e) = @_;
    my $out = "";
    for (my $i=$s;$i<=$e;$i++){
	$out .=$chars->[$i];
    }
    return &replace($out);
}

sub replace {
    my $s = shift @_;
    foreach my $r (@LocalRegExpReplace){
	eval ($s =~ $r);
    }
    return $s;
}
    
##get next word from a starting offset
sub get_next_word {    
    my ($chars,$rout,$start) = @_;
    my $n = (scalar @$chars) -1;
    my $i;
    my $idx = $start;
    my $ord;
    for ($i=$start;$i<= $n;$i++){
	$ord = ord($chars->[$i]);
	if ( $ord == 32 ||
	    $ord == 10 || 
	    $ord == 11 || 
	    $ord == 12 || 
	    $ord == 13 || 
	    $ord == 9) 
	{ 
	    $idx++;
	} else {
	    last;
	}
    }    
    my $out = "";
    for($i=$idx; $i<=$n; $i++ ) {
        $ord = ord($chars->[$i]); 
	if ($ord != 32 &&
	    $ord != 10 && 
	    $ord != 11 && 
	    $ord != 12 && 
	    $ord != 13 && 
	    $ord != 9) 
	{ 
	    $out .= $chars->[$i];
	} else {
	    last;
	}
    }
    $$rout = $out;
    if($out eq "") {
	return -1;
    }
    return $idx;
}

##load tokenization configuration file
sub load_conf {
    my $line;
    my $section="";
    my $abbr = "";
    my $punc = "";
    my @abbr = ();
    open(IN_CONF,"$_Conf") or die("Cannot open $_Conf for reading!\n");
    while($line=<IN_CONF>){
	$line = &trim($line);
	if ($line =~ /^\#:(\S+)/){
	    $section = lc $1;
	}
	elsif ($line =~ /^\#(\S+)/){
	    next;
	}
	else{
	    if ($section eq "token"){
		push @RegExpBreak, qr/$line/;
	    }
	    elsif($section eq "punctuation"){
		$punc = $line
		}
	    elsif($section eq "abbreviation"){
		push @abbr, $line;
	    }
	    elsif($section eq "replacement"){
		if ($line =~ /^(.*)\t(.*)/){
		    my ($s,$r) = ($1,$2);
		    $r='"'.$r.'"';
		    $r=~ s/(\$\d+)/".$1."/g;
		    push @LocalRegExpReplace, [qr/$s/,$r];
		}
	    }
	    elsif($section eq "global"){
		if ($line =~ /^s(\S)(.*)\1(.*)\1(.*)$/){
		    push @GlobalRegExpSearch, qr/$2/;
		    push @GlobalRegExpReplace, $line;
		}
	    }
	}
    }
    #sort the abbreviations from longest to shortest because of perl's eagerness
    if (@abbr){
	foreach my $ab (reverse sort {length($a) <=> length($b)} @abbr){
	    $ab =  quotemeta $ab;
	    if ($abbr eq ""){
		$abbr = "^($ab";
	    }
	    else{
		$abbr .= "|" . $ab ;
	    }
	}
	$abbr .=")";
	#priority to abbreviations 
	unshift @RegExpBreak, qr/$abbr/;
    }
    if ($punc){
	push @RegExpBreak, qr/$punc/;
    }
    close(IN_CONF);
}

sub trim {
    my $s = shift @_;
    $s =~ s/^\s+//;
    $s =~ s/\s+$//;
    return $s;
}



