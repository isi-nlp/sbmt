#!/bin/sh

if [ $# -lt 2 ]; then
    echo "Usage: $0 <training-file> <lm-file>" 1>&2
    exit 1
fi

E=$1
LM=$2

cd $PBS_O_WORKDIR
TMPDIR=${TMPDIR:-/tmp}

export HADOOP_HOME=/home/nlg-03/mt-apps/hadoop/0.20
PATH=$HADOOP_HOME/bin:$PATH
CLUSTER_DIR=$PBS_O_WORKDIR/hadoop.$PBS_JOBID
pbs_hadoop.py $CLUSTER_DIR || exit 1
export HADOOP_CONF_DIR=$CLUSTER_DIR/conf

# Assume already tokenized; map digits to @
cat $E | tr '0-9' '@' > $TMPDIR/e

/home/nlg-01/chiangd/biglm/kneserney/kneserney.sh $TMPDIR/e $LM 5


