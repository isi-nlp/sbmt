#!/bin/sh

if [ $# -lt 3 ]; then
  echo "usage: kneserney.sh <input> <output> <order>"
fi

INPUT=$1
OUTPUT=$2
ORDER=$3

KNDIR=/home/nlg-01/chiangd/biglm/kneserney
export HADOOP_HOME=/home/nlg-03/mt-apps/hadoop/0.20
export PATH=$KNDIR:$HADOOP_HOME/bin:$PATH
LOCALDIR=${TMPDIR:-/tmp}
SHAREDDIR=/scratch

if [ $ORDER -le 1 ]; then
  echo "Order must be at least 2"
  exit 1
fi

echo "Order: $ORDER" 1>&2
echo "Training data: $INPUT" 1>&2

PYTHON=/home/nlg-01/chiangd/pkg/python/bin/python

hadoop_stream () {
    $HADOOP_HOME/bin/hadoop jar $HADOOP_HOME/contrib/streaming/hadoop-*-streaming.jar "$@" # || exit 1
}

hadoop fs -put $INPUT train

for N in `seq $ORDER -1 1`; do
  if [ $N -eq $ORDER ]; then
    echo Counting ${N}-grams 1>&2
    hadoop_stream \
	-input train \
	-mapper "$PYTHON $KNDIR/ngrams.py -n $N" \
	-reducer "$PYTHON $KNDIR/count.py -u" \
	-output tokens.$N

    hadoop fs -mkdir extratokens.$N # empty

  else
    P=`expr $N + 1`
    # recursive Kneser-Ney

    # One of Chen and Goodman's mods: smooth the lower-order model
    # BUT using #lefttypes in place of #tokens.
    # Yes, absolute-discount.pl will calculate the discount based on
    # #lefttypes. This is what SRI-LM does by default.

    echo Counting modified ${N}-grams 1>&2

    hadoop_stream \
	-input tokens.$P \
	-input extratokens.$P \
	-mapper "$PYTHON $KNDIR/suffix.py" \
	-reducer "$PYTHON $KNDIR/count.py -u" \
	-output tokens.$N

    if [ $N -gt 1 ]; then
      # SRI-LM mixes in the unmodified counts for the lower-order
      # n-grams at beginnings of sentences. The count-counts are
      # computed from this mixed bag. This seems weird but it works.
      # The more sensible alternative, which is to prepend n-1 <s>
      # symbols to each sentence, actually gets perplexity a hair
      # worse

      echo Counting start-of-sentence events 1>&2
      hadoop_stream \
	  -input train \
	  -mapper "$PYTHON $KNDIR/ngrams.py -sn $N" \
	  -reducer "$PYTHON $KNDIR/count.py -u" \
	  -output extratokens.$N

    else
      # merge unknown zero-count in
      echo $'<unk>\t0' | hadoop fs -put - extratokens.1
    fi
  fi
done

for N in `seq 1 $ORDER`; do
  echo Counting counts 1>&2

  hadoop_stream \
      -input tokens.$N \
      -input extratokens.$N \
      -mapper "cut -f2" \
      -reducer "$PYTHON $KNDIR/count.py -u" \
      -output counts.$N

  hadoop fs -getmerge counts.$N $LOCALDIR/counts.$N
done

echo Computing discounted probabilities 1>&2
hadoop fs -getmerge tokens.1 $LOCALDIR/tokens.1
hadoop fs -get extratokens.1 $LOCALDIR/extratokens.1
cat $LOCALDIR/tokens.1 $LOCALDIR/extratokens.1 | python $KNDIR/prefix.py | python $KNDIR/absolute-discount.py $LOCALDIR/counts.1 > $LOCALDIR/discount.1
for N in `seq 2 $ORDER`; do
  echo Computing discounted probabilities 1>&2
  hadoop fs -rmr counts.$N
  hadoop fs -put $LOCALDIR/counts.$N counts.$N
  hadoop_stream \
      -input tokens.$N \
      -input extratokens.$N \
      -mapper "python $KNDIR/prefix.py" \
      -reducer "python $KNDIR/absolute-discount.py counts.$N" \
      -cacheFile counts.$N\#counts.$N \
      -output discount.$N
  hadoop fs -rmr tokens.$N # save space
done

# could repeat above for multiple corpora and linearly interpolate

for N in `seq 2 $ORDER`; do
  M=`expr $N - 1`
  echo Computing backoff weights 1>&2
  hadoop_stream \
      -input discount.$N \
      -mapper "python $KNDIR/prefix.py" \
      -reducer "python $KNDIR/add-bows.py" \
      -output bows.$M
done

# easier to recompute bows instead of performing join
for N in `seq 2 $ORDER`; do
  echo Computing backoff weights 1>&2
  hadoop_stream \
      -input discount.$N \
      -mapper "python $KNDIR/prefix.py" \
      -reducer "python $KNDIR/add-bows.py -j" \
      -output probsbows.$N
  hadoop fs -rmr discount.$N # save space
done

echo Interpolating 1-grams with uniform distribution 1>&2
cat $LOCALDIR/discount.1 | $KNDIR/interpolate-uniform.pl | hadoop fs -put - interpolate.1
for N in `seq 2 $ORDER`; do
    M=`expr $N - 1`
    echo Interpolating ${N}-grams with ${M}-grams
    hadoop_stream \
	-input probsbows.$N \
	-mapper "python $KNDIR/suffix.py" \
	-reducer NONE \
	-output /tmp/suffix.$N

    join.py -f /tmp/suffix.$N interpolate.$M -r "python $KNDIR/interpolate.py" -o interpolate.$N
    hadoop fs -rmr /tmp/suffix.$N probsbows.$N # save space
done

P=`expr $ORDER - 1`
for N in `seq 1 $P`; do
  echo Merging probabilities and backoff weights 1>&2
  join.py -ef interpolate.$N bows.$N -r "python $KNDIR/arpa.py -b" -o merge.$N
  hadoop fs -rmr interpolate.$N bows.$N # save space
done
hadoop_stream \
    -input interpolate.$ORDER \
    -mapper "python $KNDIR/arpa.py" \
    -reducer NONE \
    -output merge.$ORDER
hadoop fs -rmr interpolate.$ORDER

( echo '\data\'
  for N in `seq 1 $ORDER`; do
    hadoop fs -getmerge merge.$N $SHAREDDIR/merge.$N 1>&2
    hadoop fs -rmr merge.$N 1>&2
    echo -n "ngram $N="
    wc -l < $SHAREDDIR/merge.$N
  done
  echo ''
  for N in `seq 1 $ORDER`; do
      echo "\\${N}-grams:"
      cat $SHAREDDIR/merge.$N
      echo ""
  done
  echo '\end\'
) > $OUTPUT

