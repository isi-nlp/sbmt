#!/bin/sh

# Usage: filter_xrsdb.sh inputdb outputdb filtercmd+

export HADOOP_HOME=/home/nlg-03/mt-apps/hadoop/0.20
XRSDB=/home/nlg-02/pust/xrsdb32/bin
HIERO=/home/nlg-01/chiangd/hiero-mira
HIEROHADOOP=/home/nlg-01/chiangd/hiero-hadoop
export PATH=$HIEROHADOOP:$XRSDB:$HADOOP_HOME/bin:$PATH

WORK=$PBS_O_WORKDIR

# The directory where the Hadoop cluster will be created
CLUSTERDIR=$WORK/hadoop.$PBS_JOBID

# Create the cluster
pbs_hadoop.py $CLUSTERDIR || exit 1
export HADOOP_CONF_DIR=$CLUSTERDIR/conf

cd $WORK

PYTHON=/home/nlg-01/chiangd/pkg64/python/bin/python
PERL=/home/nlg-03/voeckler/perl/i686/bin/perl

WORKDIR=.
hadoop fs -mkdir $WORKDIR
TMPDIR=${TMPDIR-/tmp}

HADOOPSTREAM="$HADOOP_HOME/bin/hadoop jar $HADOOP_HOME/contrib/streaming/hadoop-*-streaming.jar -cmdenv PYTHONPATH=$HIERO:$HIERO/lib -cmdenv LD_LIBRARY_PATH=$HIERO/lib"

NODES=`wc -l < $HADOOP_CONF_DIR/slaves`
MAPS_PER_NODE=10

INDIR=$1
OUTDIR=$2
shift
shift

EXCT="$@"
echo "filter command is: $EXCT"

###########################
# Extract cluster names 
# from old directory
$XRSDB/xrsdb_dump_index $INDIR | hadoop fs -put - index_dump

###########################
# Create new skeletal
# directory
$XRSDB/xrsdb_create -d $OUTDIR -f $INDIR/header

###########################
# Populate the new xrsdb
$HADOOPSTREAM \
  -input index_dump \
  -output new_index \
  -mapper "sh -c \"cd $WORK; $XRSDB/xrsdb_dump $INDIR 0 | $PYTHON $HIEROHADOOP/rulesig.py | $XRSDB/xrsdb_assignkeys -u raw_sig -f $INDIR/header | $EXCT \"" \
  -reducer "sh -c \"cd $WORK; $XRSDB/xrsdb_populate $OUTDIR\"" \
  -jobconf mapred.reduce.tasks.speculative.execution=false \
  -jobconf stream.num.map.output.key.fields=2 \
  -partitioner org.apache.hadoop.mapred.lib.KeyFieldBasedPartitioner \
  -jobconf num.key.fields.for.partition=1 \
  -jobconf mapred.map.tasks=$(($NODES * $MAPS_PER_NODE))

hadoop fs -getmerge new_index $TMPDIR/new_index

$XRSDB/xrsdb_index $OUTDIR < $TMPDIR/new_index
