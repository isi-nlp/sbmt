#!/bin/sh



WORKDIR=${TMPDIR:-/tmp}

echo "Using $WORKDIR" 1>&2

ROOT=/home/nlg-03/wang11/MIRA/mira-features

echo
IN=$WORKDIR/insert-ewords.$$
CAT=$WORKDIR/rules.$$
echo "Using $IN and $CAT" 1>&2

cat > $CAT
$ROOT/generate-insert-ewords-from-nonlex-rules.pl < $CAT > ./insertion-eword-list
$ROOT/syntax-mira-features.pl -e ./insertion-eword-list < $CAT
 
