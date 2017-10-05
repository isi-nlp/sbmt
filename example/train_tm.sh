#!/bin/bash

ROOT=$(cd $(dirname $0) && pwd)/..

TMPDIR=/tmp/train_tm.$$

if [ $# -eq 4 ]; then
    WORKDIR=$4
elif [ $# -eq 3 ]; then
    WORKDIR=$TMPDIR
else
    echo "usage: $0 <input infile> <output infile> <model file> [<tmpdir>]"
    exit 1
fi

INPUT_TRAIN_INFILE=$1
OUTPUT_TRAIN_INFILE=$2
OUTFILE=$3
PREFIX=$(basename $OUTFILE)

EPOCHS=10
INPUT_VOCAB_SIZE=5000
OUTPUT_VOCAB_SIZE=5000
NGRAM_SIZE=3

mkdir -p $WORKDIR

ADD_START_STOP=1

$ROOT/src/prepareNeuralTM --input_train_text $INPUT_TRAIN_INFILE --output_train_text $OUTPUT_TRAIN_INFILE --ngram_size $NGRAM_SIZE --input_vocab_size $INPUT_VOCAB_SIZE --output_vocab_size $OUTPUT_VOCAB_SIZE --validation_size 500 --write_input_words_file $WORKDIR/input_words --write_output_words_file $WORKDIR/output_words --train_file $WORKDIR/train.ngrams --validation_file $WORKDIR/validation.ngrams --numberize 1 --ngramize 1 --add_start_stop $ADD_START_STOP || exit 1

# DEBUGGING
$ROOT/src/prepareNeuralTM --input_train_text $INPUT_TRAIN_INFILE --output_train_text $OUTPUT_TRAIN_INFILE --ngram_size $NGRAM_SIZE --input_vocab_size $INPUT_VOCAB_SIZE --output_vocab_size $OUTPUT_VOCAB_SIZE --validation_size 500 --train_file $WORKDIR/train.words.ngrams --validation_file $WORKDIR/validation.words.ngrams --numberize 0 --ngramize 1 --add_start_stop $ADD_START_STOP || exit 1

$ROOT/src/trainNeuralNetwork --train_file $WORKDIR/train.ngrams --validation_file $WORKDIR/validation.ngrams --num_epochs $EPOCHS --input_words_file $WORKDIR/input_words --output_words_file $WORKDIR/output_words --model_prefix $WORKDIR/$PREFIX || exit 1

cp $WORKDIR/$PREFIX.$(($EPOCHS)) $OUTFILE || exit 1

# query LM on test set in batches
$ROOT/src/testNeuralNetwork --test_file $WORKDIR/train.ngrams --model_file $OUTFILE || exit 1

# query LM on test set one ngram at a time
$ROOT/src/testNeuralLM --test_file $WORKDIR/train.ngrams --model_file $OUTFILE --numberize 0 --ngramize 0 --add_start_stop 0 || exit 1 > $WORKDIR/train.ngrams.logprobs

rm -rf $TMPDIR
