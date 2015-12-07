#!/bin/bash
#PBS -k oe
#PBS -j oe

ORDER=3

INPUTS="/home/nlg-01/chiangd/kneserney/work1b /home/nlg-01/chiangd/kneserney/work2b"
WEIGHTS="0.5 0.5"
OUTPUT=/home/nlg-01/chiangd/kneserney/mix-b

PYTHON=/home/nlg-01/chiangd/pkg/python/bin/python
PBSDSH="pbsdsh -v"

VNODES=`pbsdsh -o sh -c 'echo $PBS_VNODENUM'`
N_VNODES=`echo $VNODES | wc -w`
PART='part$PBS_VNODENUM'

KNDIR=/home/nlg-01/chiangd/kneserney
export PATH=$KNDIR:$PATH

product() {
  RESULT=
  for X in $1; do
    for Y in $2; do
    RESULT="$RESULT $X$Y"
    done
  done
  echo $RESULT
}

mix.py `product "$INPUTS" /discount.1` -w "$WEIGHTS" > $OUTPUT/discount.1
for N in `seq 2 $ORDER`; do
    $PBSDSH sh -c "$PYTHON $KNDIR/mix.py `product \"$INPUTS\" /discount.$N.$PART` -w \"$WEIGHTS\" > $OUTPUT/discount.$N.$PART"
done
