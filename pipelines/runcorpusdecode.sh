#!/usr/bin/env bash
#PBS -l walltime=12:00:00                                                                                                                 
#PBS -l nodes=20                                                                                                                           
#PBS -l pmem=23g                                                                                                                           
#PBS -T allcores        
#PBS -N decode
#PBS -q isi
#
# convenience wrapper for the corpus-pipeline and decode-pipeline
# useful for running a directory-based set of experiments, where contrastive 
# experiments exist in sub-directories of baseline experiments
#
# example usage: runcorpusdecode.sh $corpus $tune $weightno
#
# assumptions:
#  - a directory named ruleset exists (possibly in an ancestor directory)
#    which was created by the ruleset pipeline
#  - $corpus.pipeline.resource exists (possibly in an ancestor directory)
#    which defines your corpus and reference translations
#  - all config files end in .pipeline.config 
#    grabs all config files in current and ancestor directories. conflicting items in
#    ancestor config files are overwritten by items in child config files
# 
# execute within a pbs multi-node job

set -e

PIPELINE=$(dirname $(readlink -f $0))
PIPESTEP=$PIPELINE/corpus-pipeline

CAUX=""

if $PIPELINE/util/findlocalpath aux.$1 ; then
    CAUX="-m $($PIPELINE/util/findlocalpath aux.$1)"
fi

corpus=$1
corpusdir=corpus-$corpus

cd $PBS_O_WORKDIR
mkdir -p $corpusdir

$PIPESTEP/run.sh $($PIPELINE/util/findlocalpath ruleset) $CAUX \
                 -s $($PIPELINE/util/findlocalpath $corpus.pipeline.resource) \
                 -c $($PIPELINE/util/gatherconfig) \
                 -n -o $corpusdir 2> $corpusdir/pipeline.log

PIPESTEP=$PIPELINE/decode-pipeline
tune=$2
weightno=$3
cmd=""
ext=""

if [ "x$weightno" != "x" ]; then
    cmd="-w $($PIPELINE/util/findlocalpath tune-$tune)/weights.$weightno"
    ext=".$weightno"
fi

decodedir=decode-${tune}-${corpus}${ext}

mkdir -p $decodedir

$PIPESTEP/run.sh $($PIPELINE/util/findlocalpath $corpusdir) $CAUX \
                 $($PIPELINE/util/findlocalpath tune-$tune) $cmd \
                -o $decodedir 2> $decodedir/pipeline.log
