#!/usr/bin/env bash
#PBS -l walltime=12:00:00                                                                                                                 
#PBS -l nodes=20:ppn=12
#PBS -n
#PBS -N decode
#PBS -q isi
#

SRCDIR=$(dirname $BASH_SOURCE)

. $SRCDIR/init.sh
echo command-line $SRCDIR/run.sh $@ 1>&2
$SRCDIR/runall $@
echo $SRCDIR/runall exited with status $? 1>&2
