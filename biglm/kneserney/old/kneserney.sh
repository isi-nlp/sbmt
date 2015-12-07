#!/bin/sh

# filenames are of the form
# <type>.<n> or <type>.<n>b
# <n> = order of n-grams
#  b  = "backoff" order, i.e., sorted from the second word instead of the first

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
function filter () { tr -d '\000-\010\016-\037'; }

# to do: gtmin option still not checked

INPUT=$1
ORDER=$2

KNDIR=/home/nlg-01/chiangd/biglm/kneserney/old

export PATH=$KNDIR:$PATH

function count () {
    time count.py "$@" -v -M 30M
}

function perlcount () {
    TMPDIR=/tmp/kneserney.$$
    rm -rf $TMPDIR
    mkdir $TMPDIR
    count.pl "$@" -d $TMPDIR -v
    sort -mk 2 $TMPDIR/*
}

echo "Training data: $INPUT"

for N in `seq $ORDER -1 1`; do
  echo Counting ${N}-grams
  if [ $N -eq $ORDER ]; then
    cat $INPUT | filter | count -sn $N > tokens.$N
    
  else
    P=`expr $N + 1`
    # recursive Kneser-Ney

    # One of Chen and Goodman's mods: smooth the lower-order model
    # BUT using #lefttypes in place of #tokens.
    # Yes, absolute-discount.pl will calculate the discount based on
    # #lefttypes. This is what SRI-LM does by default.

    echo Counting modified ${N}-grams
    cat tokens.${P}b | count -cf 3-`expr $P + 1` > lefttypes.$N

    echo Mixing in additional events
    if [ $N -gt 1 ]; then
      # SRI-LM mixes in the unmodified counts for the lower-order n-grams
      # at beginnings of sentences. This seems weird but it works
      cat $INPUT | filter | count -sf1-${N} > extratokens.$N
    else
      # merge unknown zero-count in
      echo $'0\t<unk>' > extratokens.1
    fi
    time sort -m -k 2 lefttypes.$N extratokens.$N > tokens.$N
  fi

  echo Re-sorting
  time sort -k 3 tokens.${N} > tokens.${N}b
done

for N in `seq 1 $ORDER`; do
  echo Counting counts
  cat tokens.$N | count -f 1 > counts.$N

  # no smoothing
  #cat tokens.$N | normalize-probs.pl > probs.$N
  #break

  # absolute discounting
  echo Computing discounted probabilities and backoff weights
  time (cat tokens.$N | absolute-discount.pl counts.$N) > discount.${N}
done

# special case: do the interpolation between 1-grams and uniform
# distribution now.

# we do this to hide "0-grams" from the outside world
# this is especially useful for linear interpolation

echo Interpolating 1-grams with uniform distribution
mv discount.1 discountorig.1
cat discountorig.1 | interpolate-uniform.pl > discount.1


