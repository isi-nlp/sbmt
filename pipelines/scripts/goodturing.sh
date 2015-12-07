#!/bin/sh

# Usage: goodturing.sh <input> <output> <key-generator>

# <key-generator> should generate three fields: 
#   x \t id \t count

# Calculates Good-Turing smoothed P(id|x)

# Assumes that a Hadoop cluster exists and HADOOP_CONF_DIR is set
# This doesn't necessarily have to be run on a node inside the cluster

if [ $# -ne 3 ]; then
  echo "usage: $0 <input> <output> <key-generator>" 1>&2
  exit 1
fi

PATH=$HADOOP_HOME/bin:$PATH
SBMTHADOOP=/home/nlg-01/chiangd/sbmt-hadoop

MYTMPDIR=${TMPDIR:-/tmp}/goodturing-$HOSTNAME-$$
mkdir -p $MYTMPDIR
hadoop fs -mkdir $MYTMPDIR

hadoop_stream () {
    $HADOOP_HOME/bin/hadoop jar $HADOOP_HOME/contrib/streaming/hadoop-*-streaming.jar -cmdenv PERL5LIB=/home/nlg-03/mt-apps/rule-ext/0.6/bin "$@" || exit 1
}

hadoop_stream -input $1 -mapper "$3" -reducer NONE -output $MYTMPDIR/keys

# Count roots
hadoop_stream \
    -input $MYTMPDIR/keys \
    -mapper /home/nlg-02/pust/keycounts/mapper \
    -reducer /home/nlg-02/pust/keycounts/reducer \
    -jobconf mapred.output.compress=false \
    -output $MYTMPDIR/keycounts

# Compute root count-counts
hadoop_stream \
    -input $MYTMPDIR/keys \
    -mapper /home/nlg-02/pust/keycountcounts/mapper \
    -jobconf stream.num.map.output.key.fields=2 \
    -jobconf mapred.output.compress=false \
    -reducer /home/nlg-02/pust/keycountcounts/reducer \
    -output $MYTMPDIR/keycountcounts

# Merge root counts and root count-counts into single files
hadoop fs -getmerge $MYTMPDIR/keycounts $MYTMPDIR/keycounts
hadoop fs -getmerge $MYTMPDIR/keycountcounts $MYTMPDIR/keycountcounts

hadoop_stream \
    -input $MYTMPDIR/keys \
    -file $MYTMPDIR/keycounts \
    -file $MYTMPDIR/keycountcounts \
    -mapper "$SBMTHADOOP/divide.py -f keycounts -g keycountcounts" \
    -reducer NONE \
    -output $2

hadoop fs -rmr $MYTMPDIR
rm -rf $MYTMPDIR

