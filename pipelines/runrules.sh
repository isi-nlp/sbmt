#!/usr/bin/env bash
#SBATCH --ignore-pbs --mem-per-cpu=1250m --ntasks=40 --ntasks-per-node=1 --cpus-per-task=8 --job-name=rules --time=48:00:00 -p isi 
#PBS -l nodes=20:ppn=8:quadcore -l walltime=96:00:00 -n -N rules -q isi
#
# convenience wrapper for the ruleset-pipeline
# useful for running a directory-based set of experiments, where contrastive 
# experiments exist in sub-directories of baseline experiments
#
# assumptions:
#  - a training file named training.pipeline.resource exists 
#    (possibly in ancestor directory), which defines the training set
#  - model files are in a directory named 'aux' 
#    (possibly in an ancestor directory)
#  - all config files end in .pipeline.config 
#    grabs all config files in current and ancestor directories. conflicting items in
#    ancestor config files are overwritten by items in child config files
#
# execute within a pbs multi-node job

set -e

PIPELINE=$(dirname $(readlink -f $BASH_SOURCE))
PIPESTEP=$PIPELINE/ruleset-pipeline
RULESET=${1:-ruleset}

#cd $PBS_O_WORKDIR
cd $(dirname $RULESET)
RULESET=$(basename $RULESET)
mkdir -p $RULESET

if ! test -d $RULESET/xsearchdb; then 
$PIPESTEP/run.sh -s $($PIPELINE/util/findlocalpath training.pipeline.resource) \
                 -m $($PIPELINE/util/findlocalpath aux) \
                 -c $($PIPELINE/util/gatherconfig) \
                 -o $RULESET \
                -nk 2> $RULESET/pipeline.log 
else 
  echo "xsearchdb already present. to re-run, delete xsearchdb"
fi
