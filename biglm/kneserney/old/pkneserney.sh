#!/bin/sh
#PBS -k oe
#PBS -j oe

ORDER=5
INPUT=$1
OUTPUT=$2

if [ $ORDER -le 1 ]; then
  echo Use kneserney.sh instead.
  exit
fi

echo "Order: $ORDER" 1>&2
echo "Training data: $INPUT" 1>&2

WORKDIR=/scratch
#WORKDIR=/home/nlg-01/chiangd/kneserney/work
LOCALDIR=$TMPDIR
#LOCALDIR=/home/nlg-01/chiangd/kneserney/work
cd $WORKDIR

KNDIR=/home/nlg-01/chiangd/biglm/kneserney/old
export PATH=$KNDIR:$PATH

# filenames are of the form
# <type>.<n> or <type>.<n>b
# <n> = order of n-grams
#  b  = "backoff" order, i.e., sorted from the second word instead of the first

# we hash on the first word (second word for backoff order) only, to save
# some regrouping.

# the file format is
# <value> tab <word> (space <word>)* (tab <bow>)?

# Because we sort these files using string comparisons,
# we need to make sure, first, that all sorts are done 
# on raw bytes:
export LC_ALL=C
# and then that whitespace characters compare less than
# all non-whitespace characters, which we accomplish
# by simply filtering out all non-whitespace control
# characters:
#cat $INPUT | tr -d '\000-\010\016-\037' | tr A-Z0-9 a-z@ > input
cat $INPUT | tr -d '\000-\010\016-\037' | tr 0-9 @ > input
#cat $INPUT | tr -d '\000-\010\016-\037' > input
INPUT=$WORKDIR/input

# move these into /tmp:
# tokens.n, n>=2
# extratokens.n
# backoff.nb
# lefttypes.n
# interpolate.n

# not
# counts.n
# tokens.1
# backoff.n
# interpolate.nb
# packed.n

PYTHON=/home/nlg-01/chiangd/pkg/python/bin/python
PERL=/home/nlg-01/blobs/perl/v5.8.8/bin/perl
PBSDSH="pbsdsh -v"
COUNT="$PYTHON $KNDIR/count.py -v -M 10M"
REGROUP="$PYTHON $KNDIR/regroup.py -v"
SORT="LC_ALL=C sort -S 1G"
RM="rm -f"
#RM=touch

VNODES=`pbsdsh -o sh -c 'echo $PBS_VNODENUM'`
N_VNODES=`echo $VNODES | wc -w`
PART='part$PBS_VNODENUM'
parts () {
  RESULT=
  for X in $VNODES; do
    RESULT="$RESULT $1.part$X"
  done
  echo $RESULT
}
PCOUNT="$COUNT -p \$PBS_VNODENUM:$N_VNODES"
PREGROUP="$REGROUP -p \$PBS_VNODENUM:$N_VNODES"

for N in `seq $ORDER -1 1`; do
  if [ $N -eq $ORDER ]; then
    echo Counting ${N}-grams 1>&2
    time $PBSDSH sh -c "$PCOUNT -sn $N -k 1 $INPUT > $WORKDIR/tokens.$N.$PART"
  else
    P=`expr $N + 1`
    # recursive Kneser-Ney

    # One of Chen and Goodman's mods: smooth the lower-order model
    # BUT using #lefttypes in place of #tokens.
    # Yes, absolute-discount.pl will calculate the discount based on
    # #lefttypes. This is what SRI-LM does by default.

    # Need * w_2...w_n to be grouped together
    echo Counting modified ${N}-grams 1>&2
    time $PBSDSH sh -c "cd $LOCALDIR; cat tokens.${P}b.$PART | $COUNT -cf 3-`expr $P + 1` > lefttypes.$N.$PART"

    if [ $N -gt 1 ]; then
      # SRI-LM mixes in the unmodified counts for the lower-order
      # n-grams at beginnings of sentences. The count-counts are
      # computed from this mixed bag. This seems weird but it works.
      # The more sensible alternative, which is to prepend n-1 <s>
      # symbols to each sentence, actually gets perplexity a hair
      # worse

      echo Counting start-of-sentence events 1>&2
      time $PBSDSH sh -c "$PCOUNT -sf1-$N -k 1 $INPUT > $LOCALDIR/extratokens.$N.$PART"

    else
      # merge unknown zero-count in
      echo $'0\t<unk>' > extratokens.1
      $PBSDSH sh -c "$PREGROUP -k 2 $WORKDIR/extratokens.1 > $LOCALDIR/extratokens.1.$PART"
    fi

    echo Mixing in additional events 1>&2
    time $PBSDSH sh -c "cd $LOCALDIR; $SORT -m -k 2 lefttypes.$N.$PART extratokens.$N.$PART > $WORKDIR/tokens.$N.$PART; $RM lefttypes.$N.$PART"
  fi

  echo Re-sorting 1>&2
  time $PBSDSH sh -c "cd $WORKDIR; $PREGROUP -k 3 `parts tokens.$N` | $SORT -k 3 > $LOCALDIR/tokens.${N}b.$PART"
done

for N in `seq 1 $ORDER`; do
  echo Counting counts 1>&2
  time $PBSDSH sh -c "$COUNT -f 1 $WORKDIR/tokens.$N.$PART > $WORKDIR/counts.$N.$PART"
  time $COUNT -m `parts counts.$N` > counts.$N

  # absolute discounting
  # For this step, we need all the ngrams w_1...w_{n-1} * to be grouped together
  echo Computing discounted probabilities and backoff weights 1>&2
  if [ $N -eq 1 ]; then
      # unigram counts are not grouped correctly for this step
      # while we're at it, interpolate in the uniform distribution
      sort -m -k 2 `parts tokens.1` | $PERL $KNDIR/absolute-discount.pl counts.1 | interpolate-uniform.pl > $OUTPUT/discount.1
  else
      time $PBSDSH sh -c "cd $WORKDIR; cat tokens.$N.$PART | $PERL $KNDIR/absolute-discount.pl counts.$N > $OUTPUT/discount.$N.$PART; $RM tokens.$N.$PART"

  fi
done

