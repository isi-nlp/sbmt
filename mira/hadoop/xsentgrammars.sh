#!/bin/sh

# usage: xsentgrammars.sh <xrsdb-dir> <src-file>

# <src-file> should be plain tokenized text, no SGML markup or lattices

LATTICE="0"
FLAGS=

while getopts "lz" OPT; do
  case $OPT in 
    l) LATTICE="1" ;;
    z) FLAGS="-s .gz" ;;
  esac
done
shift `expr $OPTIND - 1`

XRSDB=$1
SRC=$2

mkdir -p /scratch/sentgrammars

if [ `wc -l < $PBS_NODEFILE` -gt 1 ]; then
  CP="pvfs2-cp -t -n1"
else
  CP=cp
fi

XRSDB_BIN=/home/nlg-02/pust/xrsdb3/bin
PATH=$XRSDB_BIN:/home/nlg-01/chiangd/tools:$PATH

WORKDIR=/scratch/sentgrammars.$$
echo Working directory is $WORKDIR 1>&2
mkdir -p $WORKDIR

LOCALDIR=$TMPDIR/sentgrammars.$$
pbsdsh -u sh -c "mkdir -p $LOCALDIR"

rm -f $WORKDIR/chunk.*
split -l100 -da4 $SRC $WORKDIR/chunk.

echo "Generating sentence grammars" 1>&2
date 1>&2

I=0
for FILE in $WORKDIR/chunk.*; do
    if [ $LATTICE -eq "1" ]; then
      cat $FILE | /home/nlg-01/chiangd/hiero-mira/lattice.py --input-json --output-sbmt > $FILE.lat
    else
      cat $FILE | sent_to_lattice.pl --start-id=$I > $FILE.lat
    fi
    L=`wc -l < $FILE`
    echo "LD_LIBRARY_PATH=$HOME/lib:$LD_LIBRARY_PATH $XRSDB_BIN/xrsdb_batch_retrieval -i $FILE.lat -d $XRSDB -p $LOCALDIR/grammar. $FLAGS"
    I=`expr $I + $L`
done > $WORKDIR/commands

giraffe $WORKDIR/commands || exit 1

date 1>&2

echo "Transferring to /scratch" 1>&2

pbsdsh -u sh -c "for FILE in $LOCALDIR/*; do $CP \$FILE /scratch/sentgrammars/\`basename \$FILE\` 1>&2; rm -f \$FILE; done" || exit 1

pbsdsh -u rm -rf $LOCALDIR # should be empty anyway
rm -rf $WORKDIR

date 1>&2
