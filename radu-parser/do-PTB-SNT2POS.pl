#!/usr/usc/bin/perl

# take ldc.sinorama.*.ospl.eng.tok

$SOURCE="/auto/hpc-22/dmarcu/nlg/sungcpar/data/ldc.sinorama/ldc.sinorama.source";
$DEST="/auto/hpc-22/dmarcu/nlg/summer04/resources/radu-parser/radu-parser-v3.0";

print STDERR "transformMTTok2PTBTok.pl $SOURCE/$ARGV[0] > $DEST/tmp/$ARGV[0].1\n";
system("transformMTTok2PTBTok.pl $SOURCE/$ARGV[0] > $DEST/tmp/$ARGV[0].1");
print STDERR "Done transforming into PTB\n";
system("runMXPOST2.csh $DEST/tmp/$ARGV[0].1 > $DEST/tmp/$ARGV[0].2");
system("transformRatn2Coll.pl $DEST/tmp/$ARGV[0].2 > $DEST/TEST/$ARGV[0].PTB.POS");
