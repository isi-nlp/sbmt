#!/bin/sh

# Usage: condprob.sh <input> <output> <key-generator>
# Assumes that a Hadoop cluster exists and HADOOP_CONF_DIR is set
# This doesn't necessarily have to be run on a node inside the cluster

if [ $# -ne 3 ]; then
  echo "usage: $0 <input> <output> <key-generator>" 1>&2
  exit 1
fi

export HADOOP_HOME=/home/nlg-01/chiangd/pkg/hadoop
SBMTHADOOP=/home/nlg-03/mt-apps/rule-hadoop/0.2
export PATH=$SBMTHADOOP:$HADOOP_HOME/bin:$PATH

PYTHON=/home/nlg-01/chiangd/pkg/python/bin/python
PERL=/home/nlg-03/voeckler/perl/i686/bin/perl
HADOOPSTREAM="hadoop jar $HADOOP_HOME/contrib/streaming/hadoop-streaming.jar -cmdenv PERL5LIB=/home/nlg-03/mt-apps/rule-ext/0.6/bin"

# Get the info we need from the rules
# x \t y \t id \t count

$HADOOPSTREAM \
    -input $1 \
    -mapper "$3" \
    -reducer NONE \
    -output /tmp/idcount.$$ || exit 1

# Calculate the sum of counts for each x, y
# x \t y \t count

$HADOOPSTREAM \
    -input /tmp/idcount.$$ \
    -jobconf stream.num.map.output.key.fields=2 \
    -mapper /bin/cat \
    -reducer "$PYTHON $SBMTHADOOP/count.py -k2" \
    -output /tmp/countxy.$$ || exit 1

# Calculate the sum of counts for each x
# x \t count

$HADOOPSTREAM \
    -input /tmp/countxy.$$ \
    -output /tmp/countx.$$ \
    -mapper /bin/cat \
    -reducer "$PYTHON $SBMTHADOOP/count.py -k1" || exit 1

# Calculate P(y|x)
# x \t y \t count

join.py \
    /tmp/countxy.$$ \
    /tmp/countx.$$ \
    -r "$PYTHON $SBMTHADOOP/divide.py -k0" \
    -o /tmp/pyx.$$ || exit 1

hadoop fs -rmr /tmp/countxy.$$
hadoop fs -rmr /tmp/countx.$$

# Now attach P(y|x) to rule ids

join.py \
    /tmp/idcount.$$ \
    /tmp/pyx.$$ \
    -k 2 \
    -r "/bin/cut -f3,5" \
    -o $2 || exit 1

hadoop fs -rmr /tmp/idcount.$$
hadoop fs -rmr /tmp/pyx.$$
