#!/bin/sh

# wrapper for BBN calc_approx_ter

ROOT=/home/hpc-22/dmarcu/nlg
TER=$ROOT/blobs/TER_Optimizer/latest/calc_approx_ter
MTER=$ROOT/wwang/sbmt-bin/v3.0/tuning/tuning1/mter

PATH=$TER:$PATH

REFFILES="$*"
rm -f /tmp/ref.trans
for FILE in $REFFILES; do
  NEWREFFILE=/tmp/`basename $FILE`.trans
  cat $FILE | perl -e '$i=0; while (<>) {chomp; print "$_ ($i)\n"; $i++;}' >> /tmp/ref.trans
done

cat > /tmp/nbest

cat /tmp/nbest | perl -e '$i=1; while (<>) {($id, $hyp, $feats) = split(/\|\|\|/); $id =~ s/\s//g; if ($id ne $lastid) {$i=1; $lastid=$id} print "$hyp ($id:$i)\n"; $i++;}' > /tmp/nbest.trans

calc_approx_ter /tmp/ref.trans /tmp/nbest.trans /tmp/ter_stats.sum 1>&2

cat /tmp/ter_stats.sum | awk '{print "|||", $7, $8}' | paste /tmp/nbest - | perl -ne 'chomp; split(/\|\|\|/); print "$_[0] ||| $_[3] ||| $_[2]\n"'

