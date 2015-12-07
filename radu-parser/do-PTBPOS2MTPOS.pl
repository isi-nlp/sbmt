#!/usr/usc/bin/perl

# take ldc.sinorama.*.ospl.parsed.raw_radu_output.radu-tagged

$SOURCE="/auto/hpc-22/dmarcu/nlg/summer04/resources/preprocessed-corpora/chinese/parse/ldc.sinorama/data.isi_sentence_align";

$outname = $ARGV[0];
$outname =~ s/radu\-tagged/mt-tagged/;
print  "transformPTBPOS2MTPOS.pl $SOURCE/$ARGV[0]  > $SOURCE/$outname\n";
system("transformPTBPOS2MTPOS.pl $SOURCE/$ARGV[0]  > $SOURCE/$outname");
