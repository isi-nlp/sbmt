#!/bin/bash

# input:
# y x p(y|x)     z y p(z|y)

# compute:
# z x \sum_y p(y|x) p(z|y)

XY=$1
YZ=$2
OUTPUT=$3

export HADOOP_HOME=/home/nlg-03/mt-apps/hadoop/0.20
export PATH=$HADOOP_HOME/bin:$PATH
BINDIR=$(cd $(dirname $0) && pwd)

# The directory where the Hadoop cluster will be created
CLUSTERDIR=$PBS_O_WORKDIR/hadoop.$PBS_JOBID

# Create the cluster
pbs_hadoop.py $CLUSTERDIR || exit 1
export HADOOP_CONF_DIR=$CLUSTERDIR/conf

HADOOPSTREAM="hadoop jar $HADOOP_HOME/contrib/streaming/hadoop-*-streaming.jar"
PYTHON=/home/nlg-01/chiangd/pkg64/python/bin/python
JOIN=/home/nlg-01/chiangd/hadoop/join.py

cat $XY | tr ' ' '\t' | hadoop fs -put - xy
cat $YZ | tr ' ' '\t' | hadoop fs -put - yz

$HADOOPSTREAM \
    -input yz \
    -mapper org.apache.hadoop.mapred.lib.FieldSelectionMapReduce \
    -jobconf map.output.key.value.fields.spec=1,0:2 \
    -reducer NONE \
    -output yz.inverted

$JOIN -f xy yz.inverted -o joined

$HADOOPSTREAM \
    -input joined \
    -mapper org.apache.hadoop.mapred.lib.FieldSelectionMapReduce \
    -jobconf map.output.key.value.fields.spec=3,1:2,4 \
    -reducer "$PYTHON $BINDIR/compose_table.py" \
    -output composed

rm -rf $OUTPUT
hadoop fs -getmerge composed $OUTPUT
