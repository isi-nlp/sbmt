PIPELINEROOT=$(dirname $(cd $(dirname $BASH_SOURCE); pwd))
. $PIPELINEROOT/start-openmpi.sh
. $PIPELINEROOT/start-python.sh
. $PIPELINEROOT/start-hadoop-server.sh
. $PIPELINEROOT/set-traps.sh
export PATH=/home/nlg-03/mt-apps/texlive/2010/bin/x86_64-linux/:$PATH
