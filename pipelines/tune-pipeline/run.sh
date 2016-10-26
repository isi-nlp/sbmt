#!/usr/bin/env bash
#PBS -l walltime=300:00:00
#PBS -l nodes=20:ppn=12
#PBS -N tune
#PBS -q isi
#PBS -n
SRCDIR=$(dirname $BASH_SOURCE)
. $SRCDIR/init.sh
echo command-line $SRCDIR/run.sh $@ 1>&2
$SRCDIR/runall $@
echo $SRCDIR/runall exited with status $? 1>&2
