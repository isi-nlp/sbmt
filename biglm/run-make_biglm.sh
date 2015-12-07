#!/bin/sh

ulimit -c unlimited

if [ $# -lt 2 ]; then
    echo "Usage: $0 <lm-file> <noisy-lm-file>" 1>&2
    exit 1
fi

INPUT=$1
OUTPUT=$2

N=5
K=16
P=4
B=4

cd $PBS_O_WORKDIR
TMPDIR=${TMPDIR:-/tmp}

ROOT=/home/nlg-01/chiangd/biglm
PATH=$ROOT/pagh/dist:$ROOT/tools:$PATH
export LD_LIBRARY_PATH=$ROOT/pagh/dist

quantize.py $INPUT -n $N -P ${P} -B ${B} -u 0.01 > $TMPDIR/quantizer
make_biglm  $INPUT -q $TMPDIR/quantizer -k $K -o $OUTPUT

