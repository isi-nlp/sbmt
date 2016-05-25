#!/usr/bin/env bash
#PBS -l walltime=300:00:00
#PBS -l nodes=20:ppn=12
#PBS -N tune
#PBS -q isi
#PBS -n
#
# convenience wrapper for the tune-pipeline
# useful for running a directory-based set of experiments, where contrastive 
# experiments exist in sub-directories of baseline experiments
#
# example usage: runtune.sh $corpus
#
# assumptions:
#  - a directory corpus-$corpus exists (possibly in an ancestor directory)
#    which was created by the corpus-prep pipeline
#  - model files are in a directory named 'aux' 
#    (possibly in an ancestor directory)
#  - all config files end in .pipeline.config 
#    grabs all config files in current and ancestor directories. conflicting items in
#    ancestor config files are overwritten by items in child config files
#
# output:
#  a directory named tune-$corpus
#
# note:
#  you can check the progress of your tune job by periodically running
#  $PIPELINE/tune-pipeline/run.sh -po tune-$corpus
# 
# execute within a pbs multi-node job


PIPELINE=$(dirname $(readlink -f $0))
PIPESTEP=$PIPELINE/tune-pipeline
corpus=$1
corpusdir=corpus-$corpus
tunedir=${2:-tune-$corpus}

cd $PBS_O_WORKDIR
mkdir -p $tunedir

CAUX=""

if $PIPELINE/util/findlocalpath aux.$1 ; then
    CAUX="$($PIPELINE/util/findlocalpath aux.$1)"
fi

set -e 

$PIPESTEP/run.sh $($PIPELINE/util/findlocalpath ruleset) \
  -s $($PIPELINE/util/findlocalpath $corpus.pipeline.resource) \
  -c $($PIPELINE/util/gatherconfig) \
  -m $($PIPELINE/util/findlocalpath aux) $CAUX \
  -o $tunedir 2> $tunedir/pipeline.log
