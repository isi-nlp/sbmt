PIPELINEROOT=$(dirname $(cd $(dirname $BASH_SOURCE); pwd))
export PATH=/home/nlg-01/chiangd/pkg64/python/bin/:$PATH
export PYTHONPATH=$PIPELINEROOT/lib
export LD_LIBRARY_PATH=$PIPELINEROOT/lib
export DYLD_LIBRARY_PATH=$PIPELINEROOT/lib
export LANG=C
export LC_ALL=C

. $PIPELINEROOT/start-openmpi.sh
. $PIPELINEROOT/set-traps.sh

