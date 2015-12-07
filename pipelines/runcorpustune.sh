#!/usr/bin/env bash
#PBS -l walltime=300:00:00                                                                                                                
#PBS -l nodes=20                                                                                                                           
#PBS -l pmem=23g                                                                                                                           
#PBS -T allcores                                                                                                                           
#PBS -N tune
#PBS -q isi
#
# convenience wrapper for the corpus-pipeline and tune-pipeline
# useful for running a directory-based set of experiments, where contrastive 
# experiments exist in sub-directories of baseline experiments
#
# example usage: runcorpustune.sh $corpus
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
cd $PBS_O_WORKDIR

PIPELINE=$(dirname $(readlink -f $0))
PIPESTEP=$PIPELINE/corpus-pipeline

CAUX=""

if $PIPELINE/util/findlocalpath aux.$1 ; then
    CAUX="-m $($PIPELINE/util/findlocalpath aux.$1)"
fi
set -e

corpus=$1
corpusdir=corpus-$corpus

cd $PBS_O_WORKDIR
mkdir -p $corpusdir



$PIPESTEP/run.sh $($PIPELINE/util/findlocalpath ruleset) $CAUX \
                 -s $($PIPELINE/util/findlocalpath $corpus.pipeline.resource) \
                 -c $($PIPELINE/util/gatherconfig) \
                 -n -o $corpusdir 2> $corpusdir/pipeline.log

PIPESTEP=$PIPELINE/tune-pipeline
tunedir=tune-$corpus

mkdir -p $tunedir

$PIPESTEP/run.sh $corpusdir \
  -c $($PIPELINE/util/gatherconfig) \
  -m $($PIPELINE/util/findlocalpath aux) $CAUX \
  -o $tunedir 2> $tunedir/pipeline.log
