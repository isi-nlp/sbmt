#!/bin/sh

# usage: make_lex_tagged.sh <f> <e> <a> <tags> <out>

cd $PBS_O_WORKDIR

export HADOOP_HOME=/home/nlg-03/mt-apps/hadoop/0.20
export PATH=$HADOOP_HOME/bin:$PATH

# The directory where the Hadoop cluster will be created
CLUSTERDIR=$PBS_O_WORKDIR/hadoop.$PBS_JOBID
BINDIR=$(cd $(dirname $0) && pwd)

# Create the cluster
pbs_hadoop.py $CLUSTERDIR || exit 1
export HADOOP_CONF_DIR=$CLUSTERDIR/conf

HADOOPSTREAM="hadoop jar $HADOOP_HOME/contrib/streaming/hadoop-*-streaming.jar"
PYTHON=/home/nlg-01/chiangd/pkg64/python/bin/python

paste $1 $2 $3 $4 | hadoop fs -put - data

$HADOOPSTREAM \
    -input data \
    -mapper "$PYTHON $BINDIR/count_align_tagged.py $INVERSE" \
    -reducer "$PYTHON $BINDIR/divide.py -l" \
    -output table

rm -rf $5
hadoop fs -getmerge table $5 # note, this is not the same as Pharaoh order
