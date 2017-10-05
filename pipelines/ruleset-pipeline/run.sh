#!/usr/bin/env bash
#PBS -l nodes=40:ppn=8
#PBS -l walltime=96:00:00
#PBS -n
#PBS -N rules
#PBS -q isi


SRCDIR=$(dirname $BASH_SOURCE)

. $SRCDIR/init.sh
echo command-line $SRCDIR/run.sh $@ 1>&2
$SRCDIR/runall $@
echo $SRCDIR/runall exited with status $? 1>&2
