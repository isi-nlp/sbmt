package ApplicableRuleFilter;
require Exporter;
use Tree::Suffix;


use vars qw(@ISA @EXPORT @EXPORT_OK $DEBUG $xstring);
#-------------------------------------------------------------------------------
# 
# module for determining which syntax (xrs) rules match a given collection of 
# sentences.
#
# typical usage:
# @sentences = .... # load sentences into an array
# $rule = "A(x0:B C("a")) -> x0 "aa" ### id=1";
# $arf = arf_create(@sentences);
# @matches = arf_all_matches($arf,$rule);
# 
#-------------------------------------------------------------------------------
@ISA = qw(Exporter);
@EXPORT = ();
@EXPORT = (  arf_set_debug
           , arf_create
           , arf_num_phrases
           , arf_num_sentences
           , arf_all_matches
           , arf_exists_match
           , farf_create
           , farf_all_matches
           , ftarf_create
           , ftarf_all_matches
           , etree_sentence
      );

$DEBUG = 0;
$xstring = "  %x%  ";

#-------------------------------------------------------------------------------

sub arf_set_debug {
    $DEBUG = shift;
}

#-------------------------------------------------------------------------------
#
# create a new applicable-rule-filter instance.
# called as my $arf = arf_create(@sentences)
# where @sentences are the sentences to be decoded (ie after all processing
# of the input sentences is finished -- splitting, appending <foreign-sentence>,
# etc.)
#
#-------------------------------------------------------------------------------
sub arf_create 
{
    my $szref;
    @$szref = map { join(' ',split(/\s/,$_)) } @_;
    
    my $stree = Tree::Suffix->new(@$szref);

    my $arfref = { "stree" => $stree
                 , "sentences" => $szref };
                 
    return $arfref;
}

#-------------------------------------------------------------------------------
#
# create a force-decode applicable-rule-filter instance.
# called as my $farf = farf_create($target-sentences-ref, $source-sentences-ref)
#
#-------------------------------------------------------------------------------
sub farf_create
{
    my $tref = shift @_;
    my $sref = shift @_;
    my @source = @$sref;
    my @target = @$tref;
    die "mismatched sentence lengths: $#target vs $#source" 
        unless scalar @target > 0 and scalar @target == scalar @source;
    my $target_arf = arf_create(@target);
    my $source_arf = arf_create(@source);
    my $farf = { "source" => $source_arf
               , "target" => $target_arf
               };
    return $farf;
}

#-------------------------------------------------------------------------------
#
# create a force-etree-decode applicable-rule-filter instance.
# called as $ftarf = ftarf_create($target-trees-ref, $source-sentence-ref)
#
#-------------------------------------------------------------------------------
sub ftarf_create
{
    my $tref = shift @_;
    my $sref = shift @_;
    my @target = map {tokenize_etree($_)} @$tref;
    my @source = @$sref;
    return farf_create(\@target,\@source); 
}

sub farf_all_matches_helper {
    my @retarray;
    my ($farfref,$r,$lhs_proc,$rhs_proc) = @_;
    
    my ($lhs_phrase_ref,$rhs_phrase_ref) = 
        lhs_rhs_phrase_arrays_from_rule($r,$lhs_proc,$rhs_proc);
    my @lhs_matches = arf_all_matches_helper( $farfref->{"target"}
                                            , $lhs_phrase_ref );
    my @rhs_matches = arf_all_matches_helper( $farfref->{"source"}
                                            , $rhs_phrase_ref );
                                            
    foreach my $i (0 .. scalar @lhs_matches - 1) {
        my $ret = $lhs_matches[$i] && $rhs_matches[$i];
        push(@retarray, $ret);
    }
    return @retarray;
}

sub farf_all_matches {
    return farf_all_matches_helper(@_,\&lhs_phrase_array,\&phrase_array);
}

sub ftarf_all_matches {
    return farf_all_matches_helper(@_,\&lhs_tree_phrase_array,\&phrase_array);
}


#-------------------------------------------------------------------------------

sub arf_num_phrases {
    my $arfref = shift;
    return $arfref->{"num-phrases"};
}

#-------------------------------------------------------------------------------

sub arf_num_sentences {
    my $arfref = shift;
    return $arfref->{"num-sentences"};
}

#-------------------------------------------------------------------------------
#
# called as @matches = arf_all_matches($arf, $rule)
# returns an array of the same length as the number of sentences that 
# $arf was created with.
# input is an applicable-rule-filter and an xrs rule string
# $matches[x] == 0 iff $rule exactly matches sentence x
#
#-------------------------------------------------------------------------------
sub arf_all_matches {
    my ($arfref,$rule) = @_;
    my @phrase = phrase_array_from_rule($rule);
    return arf_all_matches_helper($arfref,\@phrase);
}

sub arf_all_matches_helper {
    my ($arfref,$phraseref) = @_;
    my @matches = stree_phrase_matches($arfref->{"stree"},$arfref->{"sentences"},$phraseref);
    return @matches;
}

#-------------------------------------------------------------------------------
#
# called as $match = arf_exists_match($arf, $rule)
# returns 0 iff $rule exactly matches no sentences
# returns x + 1 iff sentence x is the first sentence that matches
#
#-------------------------------------------------------------------------------
sub arf_exists_match {
    my ($arfref,$rule) = @_;
    my @phrase = phrase_array_from_rule($rule);
    return arf_exists_match_helper($arfref,\@phrase);
}

sub arf_exists_match_helper {
    my ($arfref,$phraseref) = @_;
    my @matches = stree_phrase_matches($arfref->{"stree"},$arfref->{"sentences"},$phraseref);
    foreach my $sentid (0 .. $#matches) {
        if ($matches[$sentid]) { return $sentid + 1; }
    }
    return 0;
}

#-------------------------------------------------------------------------------

sub debug {
    print STDERR join(',',@_),"\n" if $DEBUG;
}

sub phrase_array {
    my $rhs = shift(@_);
    my @retarray = ();
    my @rhsArr = split(/\s/, $rhs);
    my @rhsPhrase;
    my $numRhsPhrases=0;
    
    foreach my $rhsToken (@rhsArr) {
    if ($rhsToken =~ /^x/) {    # if its an xrs variable
        if ($#rhsPhrase>-1) {   # we've been buildiong a rhs phrase
        my $phrasestr = join(' ', @rhsPhrase);
        $numRhsPhrases++;
        push(@retarray, $phrasestr);
        @rhsPhrase = ();
        }
        push(@retarray,$xstring);
    } else { # not an xrs variable
        my $stripped = $rhsToken;
        $stripped =~ s/^\"(.*)\"$/$1/g;
        push(@rhsPhrase, $stripped);
    }
            
    } # end for each token  
    
    if ($#rhsPhrase>-1) {   # we've been building a rhs phrase
    my $phrasestr = join(' ', @rhsPhrase);
    $numRhsPhrases++;
    push(@retarray, $phrasestr);
    }
        return @retarray;
}

#-------------------------------------------------------------------------------
#  
# (x\d+): | (\"\")\" | (\"[~\"]+)\"
#
#-------------------------------------------------------------------------------
sub lhs_phrase_array {
    my $lhs = shift(@_);
    debug("lhs_phrase_array:lhs=$lhs");
    my @retarray = ();
    my @phrase = ();
    while ($lhs =~ /(x\d+):|\"(\")\"|\"([^\"]+)\"/g) {
        if (defined($1)) {
            if ($#phrase > -1) {
                push(@retarray, join(' ', @phrase));
                @phrase = ();
            }
            push(@retarray,$xstring);
        } elsif (defined($2)){
            push(@phrase,$2);
        } elsif (defined($3)) {
            push(@phrase,$3);
        }
    }
    
    if ($#phrase > -1) {
        push(@retarray, join(' ', @phrase));
    }
    return @retarray;
}

# x\d+:[^\s\)\(]+ --- an indexed non-terminal
# \"\"\"| --- special case quote-terminal
# \"[^\"]\" --- terminal
# [^\s\(\)]+\( --- a begin non-terminal
# \)  --- an end non-terminal
sub lhs_tree_phrase_array {
    my $etree = shift @_;
    my @retarray = ();
    my @phrase = ();
    while ($etree =~ /x\d+:([^\s\(\)]+)|\"(\")\"|\"([^\"]+)\"|([^\s\(\)]+)\(|(\))/g) {
        if (defined($1)) { # indexed non-terminal; break in retarray
            push @phrase, "($1";
            push @retarray, join(' ', @phrase);
            @phrase = ();
            push @retarray, $xstring;
            #push @phrase, ")";
        } elsif (defined($2)) { # terminal """
            push @phrase, $2;
        } elsif (defined($3)) { # terminal "some-word"
            push @phrase, $3
        }elsif (defined($4)) { # start of subtree
            push @phrase, "($4";
        #} elsif ($5) { # end of subtree marker
        #    push @phrase, $5; 
        } 
    }
    if (scalar @phrase > 0) {
        push @retarray, join(' ',@phrase);
    }
    return @retarray;
}

#-------------------------------------------------------------------------------
#
# separates items into tree into ")", "word", "(NonTerminal", 
# separated by whitespace
#
#-------------------------------------------------------------------------------
sub tokenize_etree_broken {
    my $etree = shift @_;
    my $first = 1;
    my $spacer = "";
    my $retval = "";
    my @tokenized = split(/\s*(\(|\))\s*|\s+/,$etree);
    debug("tokenize_etree:", join(',',@tokenized));
    my $cparen_is_terminal = 0;
    my $lookahead = 0;
    foreach my $tok (@tokenized) {
        if ($tok =~ /^\s*$/) {debug("\n\t skip"); next; }
        debug("\n\t $tok lookahead=$lookahead cparen_is_terminal=$cparen_is_terminal");
        if ($lookahead) {
            if ($tok eq ')') {
                $retval = $retval . $spacer . '(';
                $spacer = " ";
                $cparen_is_terminal = 0;
            } else {
                $retval = $retval . $spacer . "($tok";
                $spacer = " ";
                $cparen_is_terminal = 1;
            }
            $lookahead = 0;
        } elsif ($tok eq '(') {
            $lookahead = 1;
        } elsif ($tok eq ')') {
            if($cparen_is_terminal) {
                $retval = $retval . $spacer . ')';
                $spacer = " ";
                $cparen_is_terminal = 0;
            }
        } else {
            $retval = $retval . $spacer . $tok;
            $spacer = " ";
            $cparen_is_terminal = 0;
        }
    }
    debug("tokenize_etree: $etree -> $retval");
    return $retval;
}

sub tokenize_etree {
    my $etree = shift @_;
    my $retval = "";
    $etree =~ s/^\s*//;
    my @tokens = split(/\s+/,$etree);
    my @retarray;
    foreach my $tok (@tokens) {
        if ($tok =~ /^(\S+)\)$/) {
            push @retarray, $1;
        } elsif ($tok =~ /^\(\S+/) {
            push @retarray, $tok;
        }
    }
    $retval = join " ", @retarray;
    return $retval; 
}

sub etree_sentence {
    my $etree = shift @_;
    my $first = 1;
    my $spacer = "";
    my $retval = "";
    $etree =~ s/^\s*//;

    while ($etree =~ /\(\s*(\S+)\s+|(\))|([^\s\(\)]+)/g) {
        if (defined($3)) { 
            $retval = $retval . $spacer . $3;
            if ($first) {
                $spacer = " ";
                $first = 0;
            }
        }
    }  
    return $retval; 
}

#-------------------------------------------------------------------------------
sub array_cmp {
    my ($a1,$a2) = @_;
    if (scalar @$a1 != scalar @$a2) { return 0; }
    foreach my $idx (0 .. scalar(@$a1) - 1) { 
        if ($a1->[$idx] < $a2->[$idx]) { return 0; }
    }
    return 1;
}

sub advance {
    my ($string, $idx) = @_;
    my $len = length $string;
    ++$idx;
    #if ($idx >= $len) { return $len + 1; }
    if ($idx > $len) { return $len + 1; }
    my $retval = index ($string,' ',$idx);
    if ($retval == -1) { $retval = $len; }
    return $retval;
}

sub full_match {
    my ($string,$first,$past) = @_;
    if (($first == 0 or substr($string,$first-1,1) eq ' ') and
       ($past >= length($string) or substr($string,$past,1) eq ' ')) { return 1; }
    else { return 0; }
}

sub stree_phrase_matches {
    my ($stree,$sentenceref,$phraseref) = @_;
    
    my @sz = map { length($_) + 1 } @$sentenceref;
    my @pos = map {0} @sz;
    #if (0 == scalar(@$phraseref)) { @pos; }
    return @pos unless scalar @{$phraseref};
    debug("pos: @pos");
    debug("sz: @sz");
    foreach my $word (@$phraseref) {
        if ($word eq $xstring) {
            @pos = map { advance($sentenceref->[$_],$pos[$_]) } (0 .. $#pos);
            debug("pos: @pos");
        } else {
            my @newpos = @sz;
            debug("newpos-init: @newpos\n");
            debug("processing: $word");
            foreach my $stree_result ($stree->search($word)) {
                debug("process: @$stree_result");
                my $sentid = $stree_result->[0];
                my $past = $stree_result->[2] + 1;
                my $first = $stree_result->[1];
                if ($pos[$sentid] <= $first and 
                    $newpos[$sentid] > $past and 
                    full_match($sentenceref->[$sentid],$first,$past)) {
                    $newpos[$sentid] = $past;
                }
            }
            debug("new-pos: @newpos\n");
            @pos = @newpos;
            if ( array_cmp(\@pos,\@sz) ) { return map {0} @sz; }
        }
    }
    my @retval;
    foreach my $idx (0 .. $#pos) {
        if ($pos[$idx] < $sz[$idx]) { $retval[$idx] = 1; }
        else { $retval[$idx] = 0; }
    }
    debug("stree_phrase_match: @retval");
    return @retval;
}

#-------------------------------------------------------------------------------

sub set_eoa {
    my ($eoaref,$sentence,$phrase,$occurance) = @_;
    for (my $i=0; $i <= $occurance; $i++){
    if (!defined($eoaref->[$sentence]->{$i}->{$phrase})){
        $eoaref->[$sentence]->{$i}->{$phrase} = $occurance;
    }
    }
    return $eoaref;
}
    
#-------------------------------------------------------------------------------

sub get_eoa {
    my ($eoaref,$sentence,$phrase,$after) = @_;
    if (!defined($eoaref->[$sentence]->{$after}->{$phrase})){ return -1; }
    return $eoaref->[$sentence]->{$after}->{$phrase};
}

#-------------------------------------------------------------------------------

sub phrase_array_from_rule {
    my ($original_line) = @_;
    my @retval = [];
    if (/^(([^( ]+)\(.*\)) -> (.*) ### (.*)/o) {
    my $rhs = $3;
    @retval = phrase_array($rhs);
    debug("$rhs ==> @retval\n");
    }
    return @retval;
}

sub lhs_rhs_phrase_arrays_from_rule
{
    my ($original_line, $lhs_phrase_extract, $rhs_phrase_extract) = @_;
    my $lhs_array_ref; @$lhs_array_ref = ();
    my $rhs_array_ref; @$rhs_array_ref = ();
    
    if (/^(([^( ]+)\(.*\)) -> (.*) ### (.*)/o) {
        my $lhs = $1;
        my $rhs = $3;
        @$lhs_array_ref = &$lhs_phrase_extract($lhs);
        @$rhs_array_ref = &$rhs_phrase_extract($rhs);
        debug("$lhs ==> @$lhs_array_ref");
        debug("$rhs ==> @$rhs_array_ref");
    }
    return ($lhs_array_ref,$rhs_array_ref);
    
}

#-------------------------------------------------------------------------------

1;
