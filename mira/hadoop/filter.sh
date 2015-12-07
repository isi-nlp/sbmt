#!/bin/sh

# Usage: filter.sh <input-file> <output-dir>
# Assumes that a Hadoop cluster exists and HADOOP_CONF_DIR is set
# This doesn't necessarily have to be run on a node inside the cluster

export HADOOP_HOME=/home/nlg-01/chiangd/pkg/hadoop
HIEROHADOOP=/home/nlg-01/chiangd/hiero-hadoop
export PATH=$HIEROHADOOP:$HADOOP_HOME/bin:$PATH

HIERO=/home/nlg-01/chiangd/hiero-mira.i686
PYTHON=/home/nlg-01/chiangd/pkg/python/bin/python
HADOOPSTREAM="hadoop jar $HADOOP_HOME/contrib/streaming/hadoop-streaming.jar -cmdenv PYTHONPATH=$HIERO:$HIERO/lib -cmdenv LD_LIBRARY_PATH=$HIERO/lib"

WORKDIR=.
hadoop fs -mkdir $WORKDIR

NODES=`wc -l < $HADOOP_CONF_DIR/slaves`

INFILE=$1

OUTDIR=$2
rm -rf $OUTDIR
mkdir -p $OUTDIR

echo Building per-sentence grammars for $INFILE

# remove dummy output directory that we are about to create
hadoop fs -rmr null

##################################################################
# Map:    read in a rule and write out (segid, rule) for each segid
#         that rule matches
# Reduce: (segid, rule) -> write rule to sentence grammar #segid

# Turn off speculative execution for the reducer because it
# outputs to directories, not to stdout
$HADOOPSTREAM \
    -input $WORKDIR/rules.final \
    -output null \
    -mapper "$PYTHON $HIEROHADOOP/filter.py -l6 $INFILE" \
    -reducer "$PYTHON $HIEROHADOOP/sentgrammars.py $OUTDIR" \
    -jobconf mapred.reduce.tasks.speculative.execution=false

