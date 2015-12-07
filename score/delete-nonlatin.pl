#!/home/nlg-01/blobs/perl/v5.8.8/bin/perl -w -CSD

push (@INC, ("/home/nlg-01/blobs/perl/v5.8.8/lib/5.8.8/i686-linux",
	     "/home/nlg-01/blobs/perl/v5.8.8/lib/5.8.8",
	     "/home/nlg-01/blobs/perl/v5.8.8/lib/site_perl/5.8.8/i686-linux",
	     "/home/nlg-01/blobs/perl/v5.8.8/lib/site_perl/5.8.8",
	     "/home/nlg-01/blobs/perl/v5.8.8/lib/site_perl"));

# run uconv -f utf8 -t utf8 --callback escape-xml before me

sub process_word {
    $word = $_[0];

    # All-Arabic or All-Chinese words are normal
    if ($word =~ /^\p{Arabic}+$/ || $word =~ /^\p{Han}+$/) {
	$word = "";
    } else {
	print STDERR "strange word: $word\n";
	$word = "";
    }

    return $word;
}

while (<>) {
    # First, rescue some non-ASCII non-Latin1 chars
    # trying to approximate the tokenizer

    # These are normal Unicode but we want to make them ASCII
    # apostrophe/single quote/glottal stop
    s/\x{2019}(s|d|ve|ll)\b/ '$1/g; # tokenize
    s/\x{2019}\b/ '/g; # probably right curly single quote, tokenize
    s/\x{2019}/'/g; # otherwise don't tokenize. even n't isn't tokenized for us
    s/\x{201c}/ " /g; # left curly double quote
    s/\x{201d}/ " /g; # right curly double quote
    s/\x{2014}/ -- /g; # en dash

    # This is not normal Unicode
    s/ \x{f818} / - /g;
    s/\x{f818}/ \@-\@ /g;

    # Change all whitespace to space
    s/\s+/ /g;

    # Anything unprintable or non-UTF-8, or not in the ASCII/Latin1 block of UTF8
    s/(\S*(\P{IsPrint}|[\x{0100}-\x{ffff}]|&#x[0-9A-F]+;)\S*)/process_word($1)/ge;

    print; print "\n";

}
