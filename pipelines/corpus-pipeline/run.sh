#!/usr/bin/env bash
SRCDIR=$(dirname $BASH_SOURCE)

. $SRCDIR/init.sh
echo command-line $SRCDIR/run.sh $@ 1>&2
$SRCDIR/runall $@
echo $SRCDIR/runall exited with status $? 1>&2
