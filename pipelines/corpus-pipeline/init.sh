PIPELINEROOT=$(readlink -f $(dirname $(cd $(dirname $BASH_SOURCE); pwd)))
. $PIPELINEROOT/set-traps.sh
. $PIPELINEROOT/start-python.sh
. $PIPELINEROOT/start-hadoop-server.sh $1
