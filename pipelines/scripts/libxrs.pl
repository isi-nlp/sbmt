use green;
use strict;
use warnings;

my $cls_punc=q(!-/:-@\[-`{-~);
my $cls_num=q{0-9.,\-:};
my $cls_alpha=q{a-zA-Z};
my $cls_text=$cls_alpha.$cls_num.$cls_punc;

sub pos_if_cd {
    local ($_)=@_;
    if (/^[$cls_num]*$/o && !/^[$cls_punc]*$/o) {
        return "CD";
    } else {
        return undef;
    }
}

sub pos_if_cd_punc {
    local ($_)=@_;
    if (/^[$cls_punc]*$/o) {
        return punc_pos($_,':');
    } elsif (/^[$cls_num]*$/o) {
        return "CD";
    } else {
        return undef;
    }
}

sub safe_xrs_lexical_quote
{
    local ($_)=@_;
    s/ /_/g;
    s/"/''/g if length>1;
    return qq{"$_"};
}

sub safe_pos {
    local($_)=@_;
    s/\(/-LRB-/;
    s/\)/-RRB-/;
    s/"/''/g;
    s/-\>/-/g;
    return $_;
}

my %brilltags=();

sub assertTag {
    my ($tag)=@_;
    die "ILLEGAL POS TAG $tag" if defined($tag) && $tag =~ /[()"]/;
}

sub fixTagNoisy {
    my ($rtag)=@_;
    if (defined($$rtag) && $$rtag =~ /[()"]/) {
        warning("ILLEGAL POS TAG $$rtag");
        $$rtag=safe_pos($$rtag);
    }
}

sub getBrillTagDefault {
    my ($word,$tagdef,$tagmapref)=@_;
    $word=lc($word);
    $tagmapref=\%brilltags unless defined $tagmapref;
    my $tag=$tagmapref->{$word};
    $tag=green::getDefaultTag($word,$tagdef) unless $tag;
    fixTagNoisy(\$tag);
    info_remember_quiet("Assigned TAG=$tag");
    return $tag;
}

sub leaf_for_eword {
    my ($word,$tagdef,$tagmapref)=@_;
    $tagmapref=\%brilltags unless defined $tagmapref;
    my $tag=pos_if_cd_punc($word);
    $tag=$tagmapref->{$word} unless $tag;
    $tag=getDefaultTag($word) unless $tag;
    return $tag.'('.safe_xrs_lexical_quote($word).')';
}

sub xrs_phrasal_rule
{
    my ($eref,$fref,$tagphrase,$tagdef,$tagmapref) = @_;
    local $,=' ';
    my @equotes=map { &leaf_for_eword($_,$tagdef,$tagmapref) } @$eref;
    my @fquotes=map { &safe_xrs_lexical_quote($_) } @$fref;
    my $lhs=(scalar @equotes) > 1 ? "$tagphrase(@equotes)" : "@equotes";
    &debug("phrase e/f,lhs:",@equotes,@fquotes,$lhs);
    return  "$lhs -> @fquotes ###";
}

sub xrs_lexical_rule
{
    my ($tag,$word,$foreign)=@_;
    $foreign=$word unless defined $foreign;
    return $tag.'('.safe_xrs_lexical_quote($word).') -> '.safe_xrs_lexical_quote($foreign).' ###';
}

sub read_vocab_counts {
    my ($file,$href)=@_;
    local *HASH;
    open HASH,'<',$file or die "Couldn't read file $file: $!";
    local $_;
    while(<HASH>) {
        chomp;
        my ($id,$word,$count)=split;
        info_remember_quiet("word with underscore: $word") if $word =~ /_/;
        $href->{$word}=$count;
    }
    close HASH;
}

sub read_brill_pos_tags {
    my ($tagfile,$tagmapref)=@_;
    $tagmapref=\%brilltags unless defined $tagmapref;
    open IN, $tagfile or die "Can't open $tagfile: $!\n";
    info("Reading POS tag file $tagfile ...");
    while (<IN>) {
        next if (/^\(.*\)\s+$/); # to ignore lines like: (3/26 1991) that appear
                                 # in the file
        /(\S+)\s+([^|\s]+)/ or die "Bad format: $_\n";
        my ($word,$tag) = ($1,$2);
        $word =~ s/\"/\'\'/g;
        if ($tag eq "\#") {
            info("$word $tag");
            $tag = "SYM";
        }
        if ($tag eq "") {
            info("$word [$tag]");
            $tag = "''";
        }
        if ($tag eq "1991)") {
            info("$word $tag");
            die;
        }
        my $lcword=lc($word); #FIXME: we've already lowercased everything by the
                              #time I see LEXICON and MANLEX :(
        my $newtag=safe_pos($tag);
        assertTag($newtag);
        my $alreadylc=$lcword eq $word;
        my $oldtag=$tagmapref->{$lcword};
        my $dup=0;
        if ($oldtag && $oldtag ne $newtag) {
            my $msg=$alreadylc ? "$oldtag -> $newtag" : "$newtag -> $oldtag";
            info_remember_quiet("Preferred lowercased-word tag in Brill POS file: ". $msg," for word $lcword");
            $dup=1 unless $alreadylc;
        }
        $tagmapref->{$lcword} = $newtag unless $dup;

        info_remember_quiet("BRILL POS=$tag","for $word") if &debugging;
    }
    close IN;
}


1;
