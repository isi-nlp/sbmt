#!/bin/sh

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/hpc-22/dmarcu/nlg/blobs/icu/latest/lib
PATH=/home/hpc-22/dmarcu/nlg/blobs/icu/latest/bin:$PATH

ROOT=/home/hpc-22/dmarcu/nlg/chiangd

if [ "x$PARALLEL" != "x" ]; then
  cat > /tmp/hiero-wrapper.$$.input
  $ROOT/sentserver/sentserver < /tmp/hiero-wrapper.$$.input > /tmp/hiero-wrapper.$$.output & 
  SENTSERVER_PID=$!
  for NODE in $NODES; do
    rsh -n $NODE "cd $RUNDIR; $ROOT/sentserver/sentclient $HOST $ROOT/hiero/decoder.py $4 -w \"$2\" -m $WORKDIR/hiero-wrapper.$$.nbest.$NODE -k $1" & #2> /tmp/hiero-wrapper.$$.$NODE.err &
  done
  wait $SENTSERVER_PID
  for NODE in $NODES; do
    cat $WORKDIR/hiero-wrapper.$$.nbest.$NODE
  done
  echo start 1-best output 1>&2
  cat /tmp/hiero-wrapper.$$.output 1>&2
  echo end 1-best output 1>&2
else
  $ROOT/hiero/decoder.py $4 -w "$2" -m - -k $1
fi | uconv -f utf8 -t ascii --callback skip

