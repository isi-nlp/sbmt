#!/usr/bin/env bash

#SBATCH --time=48:00:00 --ntasks=10 --cpus-per-task=12 --mem=18g --ntasks-per-node=1 --job-name=decode -p isi
#PBS -l walltime=48:00:00 -l nodes=10:ppn=12:hexcore -n -N decode -q isi

#
# convenience wrapper for the decode-pipeline
# useful for running a directory-based set of experiments, where contrastive 
# experiments exist in sub-directories of baseline experiments
#
# example usage: rundecode.sh $test $tune [$n]
#
# assumptions:
#  - directory corpus-$test exists (possibly in an ancestor directory) 
#    which was created by corpus-pipeline
#  - tune-$tune exists (possibly in an ancestor directory), which was created by
#    tune-pipeline, and contains weights.$n or weights.final
# 
# output:
#  a directory named decode-${tune}-${corpus}[.$n]
#
# execute within a pbs multi-node job


PIPELINE=$(dirname $(readlink -f $0))
PIPESTEP=$PIPELINE/decode-pipeline
corpus=$1
tune=$2
weightno=$3
cmd=""
ext=""

#if [ "x$PBS_O_WORKDIR" != "x" ]; then
#    cd $PBS_O_WORKDIR
#fi

if [ "x$weightno" != "x" ]; then
    cmd="-w $($PIPELINE/util/findlocalpath tune-$tune)/weights.$weightno"
    ext=".$weightno"
fi

CAUX=""
if $PIPELINE/util/findlocalpath aux.$1 ; then
    CAUX="-m $($PIPELINE/util/findlocalpath aux.$1)"
fi

set -e

decodedir=${4:-decode-${tune}-${corpus}${ext}}
cd $(dirname $decodedir)
decodedir=$(basename $decodedir)
mkdir -p $decodedir

#srun cp -r $($PIPELINE/util/findlocalpath ruleset)/xsearchdb $TMPDIR
#export XSEARCHDB=$TMPDIR/xsearchdb
$PIPESTEP/run.sh $($PIPELINE/util/findlocalpath ruleset) \
                -s $($PIPELINE/util/findlocalpath $corpus.pipeline.resource) \
                -u $($PIPELINE/util/findlocalpath tune-$tune) $cmd $CAUX \
                -c $($PIPELINE/util/gatherconfig) \
                -o $decodedir 2> $decodedir/pipeline.log
