#!/usr/bin/env bash
#PBS -l walltime=12:00:00                                                                                                                 
#PBS -l nodes=20:ppn=12
#PBS -n
#PBS -N decode
#PBS -q isi
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

headget=$1
cd $2
PIPELINE=$(dirname $(readlink -f $0))
PIPESTEP=$PIPELINE/decode-pipeline
corpus=$3
tune=$4
weightno=$5
cmd=""
ext=""
#echo hello
if [ "x$weightno" == "x" ]; then
   weightno='final'
fi
cmd="-w $($PIPELINE/util/findlocalpath tune-$tune)/weights.$weightno"
ext=".$weightno"
#echo hello2
#echo hello3
corpusdir=corpus-$corpus
decodedir=decode-${tune}-${corpus}${ext}
#echo hello4
if test -d $decodedir && test -f $decodedir/pipeline.log && grep -e 'Traceback' $decodedir/pipeline.log; then
    rm $decodedir/pipeline.log
    scontrol requeue $(cat $decodedir/jobid)
fi
if test -d $decodedir && test -f $decodedir/status && test "x$(cat $decodedir/status)" == "xfailure"; then
   exit 2
fi
CAT=cat
HEAD=cat
if test "x$headget" == "xhead"; then
  CAT="tail -n1000"
  HEAD="tail -n1"
fi
if test -d $decodedir && test -d $decodedir/tmp; then
  if test -e $decodedir/tmp/nbest.raw && (set -o pipefail; set -e; $CAT $decodedir/tmp/nbest.raw | grep 'sent=[0-9][0-9]*' | $HEAD | gzip -f); then 
    exit 1
  elif test -e $decodedir/nbest.raw && (set -o pipefail; set -e; $CAT $decodedir/nbest.raw | grep 'sent=[0-9][0-9]*' | $HEAD | gzip -f); then
    exit 0
  else
    exit 1
  fi
else
  exit 1
fi
