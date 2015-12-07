#!/bin/sh

# Usage: extract.sh
# Assumes that a Hadoop cluster exists and HADOOP_CONF_DIR is set
# This doesn't necessarily have to be run on a node inside the cluster

export HADOOP_HOME=/home/nlg-01/chiangd/pkg/hadoop
HIEROHADOOP=/home/nlg-01/chiangd/hiero-hadoop
export PATH=$HIEROHADOOP:$HADOOP_HOME/bin:$PATH
. /home/nlg-01/chiangd/hadoop/hadoop-utils.sh

HIERO=/home/nlg-01/chiangd/hiero-mira.i686
PYTHON=/home/nlg-01/chiangd/pkg/python/bin/python
HADOOPSTREAM="hadoop jar $HADOOP_HOME/contrib/streaming/hadoop-streaming.jar -cmdenv PYTHONPATH=$HIERO:$HIERO/lib -cmdenv LD_LIBRARY_PATH=$HIERO/lib"

# Where all the input files live
DATA=/home/nlg-02/data07/ara-eng/v5.6
COREINFO=$DATA/Training-Core/training.info
FULL=$DATA/Training-Full/training
LEX_F2N=$DATA/Training-Full/LEXICON.lef.pharaoh
LEX_N2F=$DATA/Training-Full/LEXICON.lfe.pharaoh

WORKDIR=.
hadoop fs -mkdir $WORKDIR

NODES=`wc -l < $HADOOP_CONF_DIR/slaves`
MAPS_PER_NODE=10

pack-input.py $FULL.{f-parse,e,a.f-e} | hadoop fs -put - ${WORKDIR}/full
pack-input.py $FULL.{f-parse,e,a.f-e} | subset.pl $COREINFO $FULL.info - | hadoop fs -put - ${WORKDIR}/core

##################################################################
# Rule extraction
#
# Map:    training sentence pair -> extracted (rule, count)s
# Reduce: (rule, counts) -> (rule, sum of counts)

# Hadoop will use one mapper per block (128M) of input
# but because the extractor generates so much more output than input
# (for rule extraction, about 200x; for phrase extraction, less than 10x), 
# we need to provide a better hint

hadoop_stream \
    -input $WORKDIR/core \
    -mapper "$PYTHON $HIEROHADOOP/extractor.py -L15 -l6 --minvars 1 -A -u --english-loose-limit 2 --french-trees" \
    -reducer "$PYTHON $HIEROHADOOP/sumvector.py" \
    -jobconf mapred.map.tasks=`expr $NODES \* $MAPS_PER_NODE`

hadoop_stream \
    -input $WORKDIR/full \
    -mapper "$PYTHON $HIEROHADOOP/extractor.py -L6 -l6 -v0 -A -u --english-loose-limit 2 --french-trees" \
    -reducer "$PYTHON $HIEROHADOOP/sumvector.py" \
    -jobconf mapred.map.tasks=`expr $NODES \* $MAPS_PER_NODE` 

# merge into single grammar

hadoop_stream -reducer "$PYTHON $HIEROHADOOP/sumvector.py" 

##################################################################
# Select most frequent alignment
#
# Map:    rule -> (rule without alignment, rule)
# Reduce: (rule without alignment, rules) -> 
#                select most frequent rule and sum counts

hadoop_stream \
    -mapper "$PYTHON $HIEROHADOOP/rulepart.py rule" \
    -reducer "$PYTHON $HIEROHADOOP/maxcount.py" 

##################################################################
# Conditional probabilities
# 
# Map:     rule -> (normalization group, rule)
# Reduce:  (normalization group, rule) -> 
#                new feature = count / sum of counts

hadoop_stream \
    -mapper "$PYTHON $HIEROHADOOP/rulepart.py frhs" \
    -reducer "$PYTHON $HIEROHADOOP/condprob.py pef" 

hadoop_stream \
    -mapper "$PYTHON $HIEROHADOOP/rulepart.py erhs" \
    -reducer "$PYTHON $HIEROHADOOP/condprob.py pfe"

##################################################################
# Lexical weighting and cross attribute
#
# Map:     rule -> rule with new feature

hadoop fs -put $LEX_F2N $WORKDIR/lex.f2n
hadoop fs -put $LEX_N2F $WORKDIR/lex.n2f

hadoop_stream \
    -mapper "$PYTHON $HIEROHADOOP/lexical.py lex.f2n lef" \
    -cacheFile $WORKDIR/lex.f2n\#lex.f2n

hadoop_stream \
    -mapper "$PYTHON $HIEROHADOOP/lexical.py lex.n2f lfe" \
    -cacheFile $WORKDIR/lex.n2f\#lex.n2f 

hadoop_stream -mapper "$PYTHON $HIEROHADOOP/crossing.py"

### Convert rules back to Hiero format

hadoop_stream \
    -output $WORKDIR/rules.final \
    -mapper "$PYTHON $HIEROHADOOP/finish-rule3.py"

