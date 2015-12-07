#!/bin/sh

# Usage: xrsdb.sh outdir

# Assumes that a Hadoop cluster exists and HADOOP_CONF_DIR is set
# This doesn't necessarily have to be run on a node inside the cluster

# Input: hdfs:$WORKDIR/rules.final 
# Output: $OUTDIR

export HADOOP_HOME=/home/nlg-03/mt-apps/hadoop/0.20
XRSDB=/home/nlg-02/pust/xrsdb3/bin
HIERO=/home/nlg-01/chiangd/hiero-mira
HIEROHADOOP=/home/nlg-01/chiangd/hiero-hadoop
export PATH=$HIEROHADOOP:$XRSDB:$HADOOP_HOME/bin:$PATH

PYTHON=/home/nlg-01/chiangd/pkg64/python/bin/python
PERL=/home/nlg-03/voeckler/perl/i686/bin/perl
HADOOPSTREAM="hadoop jar $HADOOP_HOME/contrib/streaming/hadoop-*-streaming.jar -cmdenv PYTHONPATH=$HIERO:$HIERO/lib -cmdenv LD_LIBRARY_PATH=$HIERO/lib"

WORKDIR=.
hadoop fs -mkdir $WORKDIR
TMPDIR=${TMPDIR-/tmp}

NODES=`wc -l < $HADOOP_CONF_DIR/slaves`
MAPS_PER_NODE=10

OUTDIR=$1

###########################
# Generate frequency table

# xrsdb_genfreq reads rules on stdin
#               writes "word \t frequency \n" on stdout

# count.py reads "word \t frequency \n" on stdin
#          and sums frequencies of consecutive duplicate words

$HADOOPSTREAM \
  -input $WORKDIR/rules.final \
  -output $WORKDIR/freq.txt \
  -mapper "$PYTHON $HIEROHADOOP/foreign-words.py" \
  -reducer "$PYTHON $HIEROHADOOP/count.py"

###################
# Create the xrsdb

hadoop fs -getmerge $WORKDIR/freq.txt $TMPDIR/freq.txt
cat $TMPDIR/freq.txt | $XRSDB/xrsdb_makefreq -f $TMPDIR/freq
hadoop fs -put $TMPDIR/freq $WORKDIR/freq

$XRSDB/xrsdb_create -d $OUTDIR -f $TMPDIR/freq

# xrsdb_assignkeys
#   reads rules on stdin
#   writes "keyword \t signature \t rule \n" on stdout

# xrsdb_populate
#   reads "keyword \t signature \t rule \n" on stdin
#   writes rules into xrsdb

# Turn off speculative execution for the reducer because it
# outputs to directories, not to stdout

$HADOOPSTREAM \
  -jobconf mapred.reduce.tasks.speculative.execution=false \
  -input $WORKDIR/rules.final \
  -output index \
  -mapper "sh -c \"$PYTHON $HIEROHADOOP/rulesig.py | $XRSDB/xrsdb_assignkeys -u raw_sig -f freq\"" \
  -jobconf stream.num.map.output.key.fields=2 \
  -partitioner org.apache.hadoop.mapred.lib.KeyFieldBasedPartitioner \
  -jobconf num.key.fields.for.partition=1 \
  -reducer "$XRSDB/xrsdb_populate $OUTDIR" \
  -cacheFile $WORKDIR/freq\#freq 

hadoop fs -getmerge index $TMPDIR/index
$XRSDB/xrsdb_index $OUTDIR < $TMPDIR/index
