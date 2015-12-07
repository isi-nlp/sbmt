#!/bin/sh
#PBS -k oe
#PBS -j oe

ORDER=3
INPUT=/home/nlg-01/chiangd/kneserney/mix-b
OUTPUT=/home/nlg-01/chiangd/kneserney/mix-b/lm

if [ $ORDER -le 1 ]; then
  echo Use kneserney.sh instead.
  exit
fi

echo "Order: $ORDER" 1>&2

#WORKDIR=/scratch
WORKDIR=/home/nlg-01/chiangd/kneserney/work
#LOCALDIR=$TMPDIR
LOCALDIR=/home/nlg-01/chiangd/kneserney/work
cd $WORKDIR

KNDIR=/home/nlg-01/chiangd/kneserney
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
PREGROUP="$REGROUP -p \$PBS_VNODENUM:$N_VNODES"

# Align w_2...w_n in higher-order model with w_1...w_{n-1} in lower-order model

$PBSDSH sh -c "$PREGROUP -k 2 $INPUT/discount.1 | awk '{print \$_, 0;}' > $LOCALDIR/interpolate.1.$PART"

for N in `seq 2 $ORDER`; do
    echo Adding backoff weights 1>&2
    time $PBSDSH sh -c "cat $INPUT/discount.${N}.$PART | $PERL $KNDIR/add-bows.pl > $WORKDIR/backoff.${N}.$PART"

    echo Re-sorting 1>&2
    # could make regroup.py do the sorting too
    time $PBSDSH sh -c "cd $WORKDIR; $PREGROUP -k 3 `parts backoff.$N` | $SORT -k 3 > $LOCALDIR/backoff.${N}b.$PART"
    $RM `parts backoff.$N`

    M=`expr $N - 1`
    echo Interpolating ${N}-grams with ${M}-grams 1>&2
    time $PBSDSH sh -c "cd $LOCALDIR; $PERL $KNDIR/interpolate.pl backoff.${N}b.$PART interpolate.$M.$PART > $WORKDIR/interpolate.${N}b.$PART" #; $RM backoff.${N}b.$PART"
    echo Re-sorting 1>&2
    time $PBSDSH sh -c "cd $WORKDIR; $PREGROUP -k 2 `parts interpolate.${N}b` | $SORT -k 2 > $LOCALDIR/interpolate.$N.$PART"
    $RM `parts interpolate.${N}b`
done

# Align w_1...w_{n-1} in higher-order model with w_1...w_{n-1} in lower-order model
for N in `seq 1 $ORDER`; do
    echo "Packing $N-gram backoff weights" 1>&2
    if [ $N -lt $ORDER ]; then
	P=`expr $N + 1`
	time $PBSDSH sh -c "cd $LOCALDIR; $PERL $KNDIR/pack-bows.pl interpolate.$N.$PART interpolate.$P.$PART > packed.$N.$PART"
    else
	time $PBSDSH sh -c "cd $LOCALDIR; $PERL $KNDIR/pack-bows.pl interpolate.$N.$PART > packed.$N.$PART"
    fi
done

for N in `seq $ORDER -1 1`; do
    echo "Packing $N-gram backoff weights" 1>&2
    if [ $N -lt $ORDER ]; then
	P=`expr $N + 1`
	time $PBSDSH sh -c "cd $WORKDIR; $PERL $KNDIR/prune.pl $N $LOCALDIR/packed.$N.$PART pruned.$P.$PART > pruned.$N.$PART"
    else
	time $PBSDSH sh -c "$PERL $KNDIR/prune.pl $N $LOCALDIR/packed.$N.$PART > $WORKDIR/pruned.$N.$PART"
    fi
done

echo "Creating language model: $OUTPUT" 1>&2
(
    echo '\data\'
    for N in `seq 1 $ORDER`; do
	echo -n "ngram $N="
	cat `parts pruned.$N` | wc -l
    done
    echo ""

    for N in `seq 1 $ORDER`; do
      echo "\\${N}-grams:"
      #sort -m -k 2 `parts pruned.$N`
      cat `parts pruned.$N`
      echo ""
    done

    echo '\end\'
) > $OUTPUT

