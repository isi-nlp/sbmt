use warnings;
use strict;

my @orig2chunksent;
my %chunksent2orig;
my @sort2orig;
my @orig2sort;

sub clear_corpus_map {
    @orig2chunksent=();
    %chunksent2orig=();
    @sort2orig=();
    @orig2sort=();
}

sub verify_corpus_map {
    my $i=0;
    for (@sort2orig) {
        my $s=$orig2sort[$_];
        die "sorted line $_ doesn't agree when mapped back from original $orig2sort[$_]: $s != $i" unless $i == $s;
        ++$i;
    }
    $i=0;
    for (@orig2chunksent) {
        my $s=$chunksent2orig{$_};
        die "sorted line $_ doesn't agree when mapped back from original $chunksent2orig{$_}: $s != $i" unless $i == $s;
        ++$i;
    }
}

sub read_corpus_map {
    &clear_corpus_map;
    my ($fh)=@_;
    local $_;
    while (<$fh>) {
        next if /^\#/;
        /(\d+)\s+(\d+:\d+)\s+(\d+)/ or die "bad corpus-map format.  expected [sorted-0-index] [sorted-chunk:sentno] [orig-0-index]";
        my ($sort,$chunksent,$orig)=($1,$2,$3);
        $sort2orig[$sort]=$orig;
        $orig2sort[$orig]=$sort;
        $chunksent2orig{$chunksent}=$orig;
        $orig2chunksent[$orig]=$chunksent;
    }
    &verify_corpus_map;
}


sub load_corpus_map {
    my ($file)=@_;
    open CMFILE,"<",$file or die "couldn't open corpus-map file $file";
    read_corpus_map(\*CMFILE);
}

sub sort_to_orig {
    return \@sort2orig;
}

sub orig_to_sort {
    return \@orig2sort;
}

sub chunksent_to_orig {
    return \%chunksent2orig;
}

sub sorted_to_original {
    return $sort2orig[$_[0]];
}

sub original_to_sorted {
    return $orig2sort[$_[0]];
}

sub chunksent_to_original {
    return $chunksent2orig{$_[0]};
}

sub original_to_chunksent {
    return $orig2chunksent[$_[0]];
}

sub line_chunk_to_original {
    my ($line,$chunk)=@_;
    $chunk ? chunksent_to_original(chunksent($chunk,$line)) :
      sorted_to_original($line);
}

sub chunksent {
    return (0+$_[0]).':'.(0+$_[1]);
}

sub chunksent_to_sorted {
    return original_to_sorted(&chunksent_to_original);
}

sub chunksent_split {
    local ($_)=@_;
    /^(\d+):(\d+)$/ && return ($1,$2);
}

1;
