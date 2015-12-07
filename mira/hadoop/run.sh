#!/bin/sh
#PBS -l nodes=20:disk60g
#PBS -l walltime=24:0:0

export HADOOP_HOME=/home/nlg-01/chiangd/pkg/hadoop
HIEROHADOOP=/home/nlg-01/chiangd/hiero-hadoop
export PATH=$HIEROHADOOP:$HADOOP_HOME/bin:$PATH
#WORK=/home/nlg-05/chiangd/experiments/gale09.zh-en
WORK=/home/nlg-05/chiangd/experiments/gale09.ar-en
NAME=overlap

# The directory where the Hadoop cluster will be created
CLUSTERDIR=$PBS_O_WORKDIR/cluster2

# Create the cluster
/home/nlg-01/chiangd/hadoop/pbs_hadoop.py $CLUSTERDIR || exit 1
export HADOOP_CONF_DIR=$CLUSTERDIR/conf
hadoop fs -mkdir .

# Rule extraction: leaves output in rules.final on DFS
extract.ar-en.sh || exit 1

# Copy full grammar out to filesystem for archival
#rm -rf $EXTRACT; hadoop fs -getmerge rules.final $EXTRACT &

xrsdb.sh $WORK/xrsdb.$NAME
