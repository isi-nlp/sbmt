PIPELINEROOT=$(dirname $(cd $(dirname $BASH_SOURCE); pwd))
. $PIPELINEROOT/set-traps.sh
. $PIPELINEROOT/start-python.sh
export PATH=/home/nlg-03/mt-apps/texlive/2010/bin/x86_64-linux/:$PATH

