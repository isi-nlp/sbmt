package green;
require Exporter;
use strict;
our @ISA = qw(Exporter);

our @EXPORT = qw(punc_pos makeNP getDefaultTag getRulesFromEnt genericizeList createRuleHash isMonth isNumerical isOrdinal );

my @list_month=(
                 "jan",
                 "january",
                 "feb",
                "febuary", # JG - common misspelling (albeit probably not from RBT)
                 "february",
                 "mar",
                 "march",
                 "apr",
                 "april",
                 "may",
                 "jun",
                 "june",
                 "jul",
                 "july",
                 "aug",
                 "august",
                "sep", # JG - is this not standard?
                 "sept",
                 "september",
                 "oct",
                 "october",
                 "nov",
                 "november",
                 "dec",
                 "december",
                );
my %ismonth;
$ismonth{$_}=1 for @list_month;

my @list_ordinal=(
                  "first",
                  "second",
                  "third",
                  "fourth",
                  "fifth",
                  "sixth",
                  "seventh",
                  "eighth",
                  "ninth",
                  "tenth",
                  "eleventh",
                  "twelfth",
                  "thirteenth",
                  "fourteenth",
                  "fifteenth",
                  "sixteenth",
                  "seventeenth",
                  "eighteenth",
                  "nineteenth",
                  "twentieth",
                  "thirtieth",
                  "fourtieth",
                  "fiftieth",
                  "sixtieth",
                  "seventieth",
                  "eightieth",
                  "ninetieth",
                  "hundredth",
                  "thousandth",
                 );
my %isordinal;
$isordinal{$_}=1 for @list_ordinal;

my @list_numerical=(
                    "one",
                    "two",
                    "three",
                    "four",
                    "five",
                    "six",
                    "seven",
                    "eight",
                    "nine",
                    "ten",
                    "eleven",
                    "twelve",
                    "thirteen",
                    "fourteen",
                    "fifteen",
                    "sixteen",
                    "seventeen",
                    "eighteen",
                    "nineteen",
                    "twenty",
                    "thirty",
                    "fourty",
                    "fifty",
                    "sixty",
                    "seventy",
                    "eighty",
                    "ninety",
                    "hundred",
                    "thousand",
                   );
my %isnumerical;
$isnumerical{$_}=1 for @list_numerical;

#usage: lookup only first character (so you get things like --, ``, etc.)
#elipses (...) is an exception.
my %puncmap=(
             ',',',', # comma(,) - used by parser

             '-',':', # colons(:) - used by parser 
             ':',':',
             ';',':',

             '(','-LRB-', # left/right bracketing punc - used by parser?
             '{','-LRB-',
             '[','-LRB-',
             ')','-RRB-',
             '}','-RRB-',
             ']','-RRB-',

             "'","''", # quotes (double singlequotes)
             '"',"''",
             '`',"``",

             '.','.', #sentence terminators (.)
             '?','.',
             '!','.',

             '%','NN', #almost always NN, sometimes JJ

              # PTB standard:
             '$','$',
             '#','#'
);

my %monthmap=(
                 "jan",1,
                 "january",1,
                 "feb",2,
                "febuary",2, # JG - common misspelling (albeit probably not from RBT)
                 "february",2,
                 "mar",3,
                 "march",3,
                 "apr",4,
                 "april",4,
                 "may",5,
                 "jun",6,
                 "june",6,
                 "jul",7,
                 "july",7,
                 "aug",8,
                 "august",8,
                "sep",9, # JG - is this not standard?
                 "sept",9,
                 "september",9,
                 "oct",10,
                 "october",10,
                 "nov",11,
                 "november",11,
                 "dec",12,
                 "december",12,
);

sub punc_pos
{
    my ($word,$default)=@_;
    return ':' if $word=~ /^\.\./; #elipsis -> :
    my ($first)=substr $word,0,1;
    if (exists $puncmap{$first}) {
        return $puncmap{$first};
    } else {
        return $default;
    }
}


# Quick ways to make rules from entities


# input: estring, cstring, assumed to be unchanged from their xml presence
# output: rulestring. formatted for inclusion in a rules file, sans probs.
#         thus, quotes added around words, an arrow, np structure.
sub makeNP {
    my ($estring, $cstring) = @_;
    my @ewords = split /\s+/, $estring;
    my @cwords = split /\s+/, $cstring;
    my $cout = "\"".join("\" \"", @cwords)."\"";
    my $eout;
    if (scalar(@ewords) == 1) {
	$eout = getDefaultTag($ewords[0])."(\"$ewords[0]\")";
    }
    else {
	# check types of tags before deciding on wrapper
	my $seenNN = 0;
	my $seenCD = 0;
	my $seenSYM = 0;
	foreach my $word (@ewords) {
	    my $tag = getDefaultTag($word); 
	    $seenNN = 1 if ($tag eq "NN");
	    $seenCD = 1 if ($tag eq "CD");
	    $seenSYM = 1 if ($tag eq "SYM");
	    $eout .= " " if ($eout);
	    $eout .= $tag."(\"".$word."\")";
	}
	$eout .= ")";
	if ($seenCD && !$seenNN) {
	    $eout = "QP($eout";
	}
	else {
	    $eout = "NPB($eout";
	}
    }
    my $rule = "$eout -> $cout";
    return $rule;
}


# based on string characteristics of an english word token get a default tag
# for most words this is NN. For "the", "a", and "an" it is DT. For anything with a number
# that does not have a letter it is CD. Classes for "$", "``", '"', "(", ")", ",", "--", ".",
# ":"and "SYM" are taken from http://www.comp.leeds.ac.uk/amalgam/tagsets/upenn.html.
# these words will usually be grouped under an NPB but there is room for QP
sub getDefaultTag {
    my ($word,$default) = @_;
    $default="NN" unless defined $default;
    my $hasalpha= $word =~ /[a-zA-Z]/;
    
    return "DT" if $word =~ /^(the|an|a)$/i;
    return "CD" if $word =~ /\d/ && !$hasalpha;
    my $tag=punc_pos($word);
    return $tag if $tag;
    return "CD" if isNumerical($word);
    return "JJ" if isOrdinal($word);
    return "NNP" if isMonth($word);
    return '$' if $word =~ /\$/;
# is "--" a real POS?
    return 'SYM' unless $hasalpha; # is SYM a good default symbol tag, or is :
                                   # better? - SYM occurs 618 times, : 2700 ...
    return $default;
}

# if a word is a number or a numerical multiplier, return true
sub isNumerical {
    my ($w) = @_;
    $w=lc($w);
    return exists($isnumerical{$w}) || $w =~ /illion$/;
}


# if a word is an ordinal from this list, return true
sub isOrdinal {
    my ($w) = @_;
    $w=lc($w);
    return exists($isordinal{$w}) || $w =~ /illionth/;
}

# month transformation utilities
# input: a string
# output: 1 if the string is an english month, 0 otherwise
sub isMonth {
    my ($w) = @_;
    # forget about periods, case, extra space
    # strip the quotes
    $w =~ s/[\.\s\"]//g;
    $w=lc($w);
    return exists($ismonth{$w});
}


#input: a list of english phrases
#output: a potentially shorter list of english phrases
# the intent is to get rid of "things we don't want" according to kevin on 4/21
sub filterEngList {
    my (@oldlist) = @_;
    my (@newlist) = ();
    foreach my $phrase (@oldlist) {
	# 1: if "associatedpress" is there, split it in twain
	$phrase =~ s/associatedpress/associated press/g;
	# TODO: complete this!

    }
    return @newlist;
}

# input:  infile:  hashable rule file 
# output: outhash: hash of those rules, indexed by type, eng compression, ch. compression
sub createRuleHash {
    my ($infile) = @_;
    my %outhash = ();
    open IN, $infile;
    while (<IN>) {
	my ($cat, $eidx, $cidx, $rule, $count) = /^(.*?)\s+:\#:\s+(.*?)\s+:\#:\s+(.*?):\#:\s+(.*?count=(\d+))\s*$/;
	$outhash{$cat}{$eidx}{$cidx}{$rule} = $count;
	# Also add an ALL member
	$outhash{ALL}{$eidx}{$cidx}{$rule} += $count;
    }
    return \%outhash;
}

# input: estring, cstring, feature type, filter_problematic, $db
#        estring and cstring are tokenized phrases. feature type is a class
#        or set of classes that may or may not be useful. filter_problematic=1 means
#        do an extra stage of returning nulls if certain word patterns are not met. 0 otherwise. 
#        $db is a hash of rules
#        indexed on type, then english (without variables), then chinese (without variables)
#        then 
# output: an array of strings, each of which is in a rule format.
sub getRulesFromEnt {
    my ($estring, $cstring, $featstring, $dofilter, $db) = @_;

    # early modification stuff
    $estring =~ s/associatedpress/associated press/g;

    # genericizeList needs quotes around words. thus:
    my @ewords = split(/\s+/, '"'.join('" "', split(/\s+/, $estring)).'"');
    my @cwords = split(/\s+/, '"'.join('" "', split(/\s+/, $cstring)).'"');
    my @efeats = split /\s+/, $featstring;
    my @feats = ();
    if (scalar @efeats == 0) {
	push @feats, 'ALL';
    }
    else {
	foreach my $feat (@efeats) {
	    push @feats, &enttype2feattype($feat);
	    # trying to identify the problem cases
	    # if a case is "problematic" return an empty array
	    # DATE and BYLINE might have number before month (29 march). This is improper and should be removed.
	    # TIMEPERIOD too

	    # modification section. always done:

	    #NUMBER+TYPE: if it matches several NUMERIC NUMERIC, throw away the first numeric
	    if ($feat eq "NUMBER+TYPE" && $estring =~ /several (\S+) (\S+)/) {
		my $firstnum = $1;
		my $secnum = $2;
		if ((isNumerical($secnum) || $secnum =~ /^\d+$/) &&
		    (isNumerical($firstnum) || isOrdinal($firstnum) || $firstnum =~ /^\d+$/)) {
		    $estring =~ s/ $firstnum//;
		    my @newewords = ();
		    my $done = 0;
		    foreach my $word (@ewords) {
			if (!$done && $word eq "\"$firstnum\"") {
			    $done = 1;
			}
			else {
			    push @newewords, $word;
			}
		    }
		    @ewords = @newewords;
		    print "Formed $estring and ".join(" ", @ewords)."\n";
		}
	    }

	    # discard section. Done when filtering is in effect

	    next unless ($dofilter);
	    if ($feat eq "DATE" ||
		$feat eq "BYLINE" ||
		$feat eq "TIMEPERIOD") {
		# algorithm: look for months and days (numbers < 32). If we see a month we can see a day. If we see
		#            a day, we may not then see a month
		my $seenMonth = 0;
		my $seenDay = 0;
		foreach my $word (@ewords) {
		    $word =~ s/\"//g;
#		    print "$word - ";
		    if (isMonth($word)) {
	#		print "month\n";
			# it's a month. if we've seen a day, die
			if ($seenDay) {
	#		    print "$estring rejected - # it's a month. if we've seen a day, die\n";
			    return ();
			}
			$seenMonth = 1;
			next;
		    }
		    if (isNumerical($word) ||
			isOrdinal($word) ||
			($word =~ /^\d{1,2}$/ && $word < 32)) {
		#	print "numerical\n";
			#it's a day. If we've seen a month, they cancel
			if ($seenMonth) {
			    $seenMonth = 0;
			    $seenDay = 0;
			}
			else {
			    $seenDay = 1;
			}
			next;
		    }
		  #  print "nothing\n";
		}
#		print "$feat: $estring accepted\n";
	    }
	    # also throw away instances with a day of the month and year that are not comma-separated
	    if ($feat eq "DATE" && $estring =~ / \d{1,2} \d{4}/) {
		print "$feat: $estring rejected - # also throw away instances with a day of the month and year that are not comma-separated\n";
		return();
	    }
		

	    if ($feat eq "NUMBER+TYPE") {
		#NUMBER+TYPE : if it matches the pattern NUMBER years and NUMBER is > 999 reject
		if ($estring =~ /\d{4} years/) {
		    print "$feat: $estring rejected - #NUMBER+TYPE : if it matches the pattern NUMBER years and NUMBER is > 999 reject\n";
		    return ();
		}
		# NUMBER+TYPE: if there's a "%" and the word before is numeric (and not a number) throw out
		if ($estring =~ /(\S+) \%/ && isNumerical($1)) {
		    print "$feat: $estring rejected - # NUMBER+TYPE: if there's a % and the word before is numeric (and not a number) throw out\n";
		    return ();
		}
		if ($estring =~ /(\S+) per\s?cent/ && $1 =~ /^[\d.\-]+$/) {
		    print "$feat: $estring rejected - something about per sent\n";
		    return ();
		}
	    }
	}
    }
    # debug check
    foreach my $feat (@feats) {
#	print "$feat does not appear as a rule type\n" unless (exists $db->{$feat});
    }


    my ($eoutptr, $coutptr, $subbed) = genericizeList(\@ewords, \@cwords, $featstring);
    my @rules = ();
    # nothing to do, so add the NP rule:
    unless ($subbed) {
	push @rules, makeNP($estring, $cstring);
	return @rules;
    }
    else {
	my @ew = @$eoutptr;
	my @cw = @$coutptr;
	
	my $ent_engstr = join ' ', @ew;
	my $ent_chstr = join ' ', @cw;
	# make them search safe
	my $orig_ent_engstr = $ent_engstr;
	my $orig_ent_chstr = $ent_chstr;
	$ent_engstr =~ s/([\\\|\(\)\{\[\^\$\*\+\?\.])/\\$1/g;
	$ent_chstr =~ s/([\\\|\(\)\{\[\^\$\*\+\?\.])/\\$1/g;
	# attempt to find the entity english in the database
	# TODO: Limit this by category if it becomes necessary. For this, use featstring
	#       and choose a cat above rather than going to ALL

	# don't allow variables to be unmatched
	foreach my $feat (@feats) {
	    foreach my $rule_engstr (keys %{$db->{$feat}}) {
		if ($rule_engstr =~ /(.*)$ent_engstr(.*)/) {
		    my $before = $1;
		    my $after = $2;
		    next if ($before =~ /@@/ or $after =~ /@@/);
		    foreach my $rule_chstr (keys %{$db->{ALL}{$rule_engstr}}) {
			if ($rule_chstr =~ /$ent_chstr/) {
			    my $before = $1;
			    my $after = $2;
			    next if ($before =~ /@@/ or $after =~ /@@/);
			    foreach my $rule (keys %{$db->{ALL}{$rule_engstr}{$rule_chstr}}) {
				push @rules, $rule;
			    }
			}
		    }
		}
	    }
	}

	# put the genericized things back in
	# note: assumes single appearance of each variable once
	# per language side!!

	for (my $i = 0; $i < scalar(@ew); $i++) {
	    if ($ew[$i] =~ /@@/) {
		for (my $j = 0; $j < scalar(@rules); $j++) {
		    $rules[$j] =~ s/$ew[$i]/$ewords[$i]/;
		}
	    }
	}
	for (my $i = 0; $i < scalar(@cw); $i++) {
	    if ($cw[$i] =~ /@@/) {
		for (my $j = 0; $j < scalar(@rules); $j++) {
		    $rules[$j] =~ s/$cw[$i]/$cwords[$i]/;
		}
	    }
	}
	# no subs made, so do the default
	if (@rules == 0) {
	    push @rules, makeNP($estring, $cstring);
	}
	return @rules;
    }
}

# input: elist, clist - arrays of english and chinese words, respectively.
# a quirk: english words are assumed to have quotation marks. This allows
# placeholders (useful for rule processing) to also appear and not be fiddled
# with.

# output: elist_out, clist_out - arrays similar to the inputs, but with certain
# words (mostly numerics and months) substituted with @@-surrounded numbered variables
#         seen_subs - a flag indicating some substitution was done
# this code is used to genericize rules and entity strings.
# the original form of this code was taken from genericize_rules_050215.pl
sub genericizeList {
    
# verbose mode for debugging rules
    my $VERBOSE_NUMERIC_1 = 0x1;
    my $VERBOSE_NUMERIC_2 = 0x2;
    my $VERBOSE_NUMERIC_3 = 0x4;
    my $VERBOSE_NUMERIC_4 = 0x8;

    my $VERBOSE_MONTH_1 = 0x10;
    my $verbose = 0
	# | $VERBOSE_NUMERIC_1  
	# | $VERBOSE_NUMERIC_2  
	# | $VERBOSE_NUMERIC_3  
	# | $VERBOSE_NUMERIC_4
	# | $VERBOSE_MONTH_1
	;

# extra debugging mode for debugging rules
    my $DEBUG_NUMERIC_1 = 0x1;
    my $DEBUG_NUMERIC_2 = 0x2;
    my $DEBUG_NUMERIC_3 = 0x4;
    my $DEBUG_NUMERIC_4 = 0x8;

    my $DEBUG_MONTH_1 = 0x10;
    my $debug = 0 
	# | $DEBUG_NUMERIC_1
	# | $DEBUG_NUMERIC_2 
	# | $DEBUG_NUMERIC_3 
	# |$DEBUG_NUMERIC_4 
	# | $DEBUG_MONTH_1
	;


# somewhat unclean way of tallying which rules fired
    my %fired_counts = ();

    my ($eptr, $cptr) = @_;
    my @ewords = @$eptr;
    my @cwords = @$cptr;
    

    # These replacement phases all involve iterating through
    # english words, finding a match, then matching to the first
    # fitting chinese word.
    
    # track  the number of subs we've seen
    my $seen_subs = 0;

    # keep track of the indices
    my $next_numeric = 0;    
    my $next_month = 0;

    # REPLACEMENT: MONTH PHASE 1:
    # complete month strings match complete and matching month strings
    # english month can be abbreviated and have periods
    for (my $i = 0; $i < scalar(@ewords); $i++) {
	my $eword = $ewords[$i];
	next unless (&isMonth($eword));
	my $cmonth = &e2cmonth($eword);
	($debug & $DEBUG_MONTH_1) && print "Debug of month phase 1: Found candidate $eword\n";
	for (my $j = 0; $j < scalar(@cwords); $j++) {
	    my $cword = $cwords[$j];
	    if ($cword eq $cmonth) {
		print "Replacement numeric phase 1: Matched $eword to $cword  \n" if ($verbose & $VERBOSE_MONTH_1);
		$ewords[$i] = "\"\@\@MONTH_".$next_month."\@\@\"";
		$cwords[$j] = "\"\@\@MONTH_".$next_month."\@\@\"";
		$seen_subs++;
		$next_month++;
		$fired_counts{MONTH_1}++;
		last;
	    }
	}
    }


    # REPLACEMENT: NUMERIC PHASE 1:
    # complete numeric strings match complete numeric strings
    # can include {. , -}. Nothing else before or after
    for (my $i = 0; $i < scalar(@ewords); $i++) {
	my $eword = $ewords[$i];
	next if ($eword =~ /\@\@/);
	next unless ($eword =~ /\"[\d\.\-\,]*\d[\d\.\-\,]*\"/);
	($debug & $DEBUG_NUMERIC_1) && print "Debug of numeric phase 1: Found candidate $eword\n";
	for (my $j = 0; $j < scalar(@cwords); $j++) {
	    my $cword = $cwords[$j];
	    next if ($cword =~ /\@\@/);
	    if ($eword eq $cword) {
		print "Replacement numeric phase 1: Matched $eword to $cword  \n" if ($verbose & $VERBOSE_NUMERIC_1);
		$ewords[$i] = "\"\@\@NUMERIC_".$next_numeric."\@\@\"";
		$cwords[$j] = "\"\@\@NUMERIC_".$next_numeric."\@\@\"";
		$seen_subs++;
		$next_numeric++;
		$fired_counts{NUMERIC_1}++;
		last;
	    }
	    ($debug & $DEBUG_NUMERIC_1) && print "Debug of numeric phase 1: No matching chinese found for candidate $eword \n";
	}
    }
    # REPLACEMENT: NUMERIC PHASE 2:
    # complete numeric strings match complete numeric strings
    # can include {. , -}. Other non-digit stuff can appear before
    # and/or after
    for (my $i = 0; $i < scalar(@ewords); $i++) {
	my $eword = $ewords[$i];
	next if ($eword =~ /\@\@/);
	next unless ($eword =~ /\"(\D*)([\d\.\-\,]*\d[\d\.\-\,]*)(\D*)\"/);
	my $e_before = $1;
	my $numstr = $2;
	my $e_after = $3;
	($debug & $DEBUG_NUMERIC_2) && print "Debug of numeric phase 2: Found candidate $eword with useful bit $numstr \n";
	# escape metacharacters
	$numstr =~ s/([\\\|\(\)\{\[\^\$\*\+\?\.])/\\$1/g;
	for (my $j = 0; $j < scalar(@cwords); $j++) {
	    my $cword = $cwords[$j];
	    next if ($cword =~ /\@\@/);
	    my $searchstr = '\"(\D*)'.$numstr.'(\D*)\"';
	    if ($cword =~ /$searchstr/) {
		my $c_before = $1;
		my $c_after = $2;
		print "Replacement numeric phase 2: Matched $eword to $cword  \n" if ($verbose & $VERBOSE_NUMERIC_2);
		$ewords[$i] = "\"$e_before\@\@NUMERIC_".$next_numeric."\@\@$e_after\"";
		$cwords[$j] = "\"$c_before\@\@NUMERIC_".$next_numeric."\@\@$c_after\"";
		$seen_subs++;
		$next_numeric++;
		$fired_counts{NUMERIC_2}++;
		last;
	    }
	}
    }
    # REPLACEMENT: NUMERIC PHASE 3:
    # complete numeric strings match complete numeric strings
    # can include {. , -}. Other non-numeric stuff can appear before
    # and/or after. trailing zeros can appear after.
    for (my $i = 0; $i < scalar(@ewords); $i++) {
	my $eword = $ewords[$i];
	next if ($eword =~ /\@\@/);
	next unless ($eword =~ /\"(\D*)([\d\.\-\,]*\d[\d\.\-\,]*)(0*\D*)\"/);
	my $e_before = $1;
	my $numstr = $2;
	my $e_after = $3;
	($debug & $DEBUG_NUMERIC_3) && print "Debug of numeric phase 3: Found candidate $eword with useful bit $numstr \n";
	# escape metacharacters
	$numstr =~ s/([\\\|\(\)\{\[\^\$\*\+\?\.])/\\$1/g;
	for (my $j = 0; $j < scalar(@cwords); $j++) {
	    my $cword = $cwords[$j];
	    next if ($cword =~ /\@\@/);
	    my $searchstr = '\"(\D*)'.$numstr.'(0*\D*)\"';
	    if ($cword =~ /$searchstr/) {
		my $c_before = $1;
		my $c_after = $2;
		print "Replacement numeric phase 3: Matched $eword to $cword  \n" if ($verbose & $VERBOSE_NUMERIC_3);
		$ewords[$i] = "\"$e_before\@\@NUMERIC_".$next_numeric."\@\@$e_after\"";
		$cwords[$j] = "\"$c_before\@\@NUMERIC_".$next_numeric."\@\@$c_after\"";
		$seen_subs++;
		$next_numeric++;
		$fired_counts{NUMERIC_3}++;
		last;
	    }
	}
    }
    # REPLACEMENT: NUMERIC PHASE 4:
    # approximate numeric strings match
    # digit sequence matches digit sequence with possible
    # markings in between
    # Other non-numeric stuff can appear before
    # and/or after
    # digits divided into largest clusters that match
    # seros can appear on the end.
    for (my $i = 0; $i < scalar(@ewords); $i++) {
	my $eword = $ewords[$i];
	next if ($eword =~ /\@\@/);
	next unless ($eword =~ /\"(\D*)([\d\.\-\,]*\d[\d\.\-\,]*)(0*\D*)\"/);
	my $e_before = $1;
	my $e_numstr = $2;
	my $e_after = $3;
	my $stripped_e_numstr = $e_numstr;
	$stripped_e_numstr =~ s/\D//g;
	# find by initial index
	my $initial = substr($stripped_e_numstr, 0, 1);
	for (my $j = 0; $j < scalar(@cwords); $j++) {
	    my $cword = $cwords[$j];
	    next if ($cword =~ /\@\@/);
	    # find something that might match the number string
	    my $searchstr = '\"(\D*)('.$initial.'[\d\.\-\,]*\d[\d\.\-\,]*)(0*\D*)\"';
	    if ($cword =~ /$searchstr/) {
		my $c_before = $1;
		my $c_numstr = $2;
		my $c_after = $3;
		# strip and see if it matches
		my $stripped_c_numstr = $c_numstr;
		$stripped_c_numstr =~ s/\D//g;
		next unless ($stripped_c_numstr eq $stripped_e_numstr);
		print "Replacement numeric phase 4: Matched $eword to $cword  \n" if ($verbose & $VERBOSE_NUMERIC_4);
		# making the match: all digits should match 1-1. What I'll do is slurp off pairs of matching digits until
		# there is a mismatch (one side has a non-dig). Then I put a placeholder on both output strings, add non-digits
		# from the side with non-dig until both match again. Continue until the strings are done.
		my $e_subbed = "";
		my $c_subbed = "";
		my @c_arr = split //, $c_numstr;
		my @e_arr = split //, $e_numstr;
		# flag to prevent phantom numerics
		my $hasmatched = 0;
		while (@c_arr && @e_arr) {
		    my $c_char = shift @c_arr;
		    my $e_char = shift @e_arr;
		    if ($c_char eq $e_char) {
			$hasmatched = 1;
			next;
		    }
		    # on mismatch, get back to match
		    if ($hasmatched) {
			$e_subbed .= "\@\@NUMERIC_".$next_numeric."\@\@";
			$c_subbed .= "\@\@NUMERIC_".$next_numeric."\@\@";
			$next_numeric++;
			$hasmatched = 0;
		    }
		    # shouldn't be a digit mismatch
		    if ($c_char =~ /\d/ and $e_char =~ /\d/) {
			die "Weirdness between $c_char and $e_char at $c_numstr and $e_numstr\n";
		    }
		    # the same code twice depending on which side has the non-digit
		    # first, the c-side
		    if ($e_char =~ /\d/) {
			while (@c_arr > 0 and $e_char ne $c_char) {
			    die "Weirdness between $c_char and $e_char: matching $e_char to $c_char\n" if ($c_char =~ /d/);
			    $c_subbed .= $c_char;
			    $c_char = shift @c_arr;
			}
			if ($e_char eq $c_char) {
			    $hasmatched = 1;
			    next;
			}
			# if they don't end together something is strange
			die "Weirdness between $c_char and $e_char: matching $e_char to $c_char\n" if ($c_char =~ /d/);
		    }
		    # now the e-side
		    else {
			while (@e_arr and ($c_char ne $e_char)) {
			    die "Weirdness between $e_char and $c_char: matching $c_char to $e_char\n" if ($e_char =~ /d/);
			    $e_subbed .= $e_char;
			    $e_char = shift @e_arr;
			}
			if ($c_char eq $e_char) {
			    $hasmatched = 1;
			    next;
			}
			# if they don't end together something is strange
			die "Weirdness between $e_char and $c_char: matching $c_char to $e_char\n" if ($e_char =~ /d/);
		    }
		}
		# last addition
		if ($hasmatched) {
		    $e_subbed .= "\@\@NUMERIC_".$next_numeric."\@\@";
		    $c_subbed .= "\@\@NUMERIC_".$next_numeric."\@\@";
		    $next_numeric++;
		}
		# join on leftovers
		$e_subbed .= join "", @e_arr if (@e_arr);
		$c_subbed .= join "", @c_arr if (@c_arr);		    
		print "Debug of numeric phase 4: $e_numstr becomes $e_subbed and $c_numstr becomes $c_subbed \n" if ($debug & $DEBUG_NUMERIC_4);
		$ewords[$i] = "\"$e_before$e_subbed$e_after\"";
		$cwords[$j] = "\"$c_before$c_subbed$c_after\"";
		$seen_subs++;
		$fired_counts{NUMERIC_4}++;
		last;
	    }
	}
    }

    # ???
    # ideas: exact number match allowing for chinese versions of decimal
    # chinese-arabic number matching
		

    # TODO: OTHER GENERICIZATIONS GO HERE
    
    return (\@ewords, \@cwords, $seen_subs);
}

# input: a type associated with an entity pair
# output: one or more types associated with a rule

# this is hand-constructed and based on the decisions made in rule_extraction.pl
# so when that changes this should change
sub enttype2feattype
{
    my ($type) = @_;
    if ($type == "NUMBER+TYPE") {
	return ("MONEY");
	# TODO: percent, num years, num people, points, etc.
    } elsif ($type == "DATE") {
	return ("DATE_FULL", "DATE_YEAR_MONTH", "DATE_MONTH_DAY");
    } elsif ($type == "BYLINE") {
	return ("BYLINE");
    } elsif ($type == "NUMBER-ORD-DAYS") {
	return ("DATE_DAY" );
    } elsif ($type == "TIMEPERIOD") {
	return ("TIMEPERIOD" );
    } elsif ($type == "PLURALTIMEPERIOD") {
	return ("PLURALTIMEPERIOD" );
    } elsif ($type == "MONTH") {
	return ("MONTH" );
    } elsif ($type == "YEAR") {
	return ("YEAR" );
    } elsif ($type == "POSSIBLEYEAR") {
	return ("YEAR" );
    } elsif ($type == "YEARREF") {
	return ("YEAR" );
    }
    return ();
# template for more
#    } elsif ($type == ) {
#	return ( );
}

# input: an english string
# output: up to one chinese string
# purpose: translate english months into chinese months by hand
sub e2cmonth {
    my ($w) = @_;
    # forget about periods, case, extra space
    # strip the quotes
    $w =~ s/[\.\s\"]//g;
    $w =~ lc($w);
    my $month=$monthmap{$w};
    return "\"$monthÔÂ\"" if ($month);
    return "";
}

1;
