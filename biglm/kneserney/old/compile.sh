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

OUTPUT=$1
ORDER=$2

KNDIR=/home/nlg-01/chiangd/biglm/kneserney/old

export PATH=$KNDIR:$PATH

cat discount.1 | awk '{print $_, 0;}' > interpolate.1

for N in `seq 2 $ORDER`; do
    echo Adding backoff weights and re-sorting
    time cat discount.${N} | add-bows.pl | sort -k 3 > backoff.${N}b

    M=`expr $N - 1`
    echo Interpolating ${N}-grams with ${M}-grams
    time interpolate.pl backoff.${N}b interpolate.$M > interpolate.${N}b
    time sort -k 2 interpolate.${N}b > interpolate.${N}
done

echo "Reorganizing backoff weights"

for N in `seq 1 $ORDER`; do
    if [ $N -lt $ORDER ]; then
	time pack-bows.pl interpolate.${N} interpolate.`expr $N + 1` > packed.$N
    else
        time pack-bows.pl interpolate.${N} > packed.$N
    fi
done

echo "Pruning useless probabilities"

for N in `seq $ORDER -1 1`; do
    if [ $N -lt $ORDER ]; then
	time prune.pl $N packed.${N} pruned.`expr $N + 1` > pruned.$N
    else
        time prune.pl $N packed.${N} > pruned.$N
    fi
done

echo "Creating language model: $OUTPUT"
(
    echo '\data\'
    for N in `seq 1 $ORDER`; do
	echo -n "ngram $N="
	wc -l < pruned.$N
    done
    echo ""

    for N in `seq 1 $ORDER`; do
      echo "\\${N}-grams:"
      cat pruned.$N
      echo ""
    done

    echo '\end\'
) > $OUTPUT


