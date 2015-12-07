#!/bin/bash
cd `dirname $0`
B=${1:-../bin/$ARCH/forest-em}
logbase=logs
timestamp=`date +%C%y%m%d_%H_%M_%S`
summarize="head -20"
diff="diff"
mkdir -p $logbase
info=$logbase/info.$timestamp
log=$logbase/log.$timestamp
out=$logbase/outparam.$timestamp
viterbi=$logbase/viterbi.$timestamp
timing=$logbase/timing.$timestamp
forest=regress-forest
norm=regress-norm
opts="-i 10000 -l - -e -10 -d 0"
CMD="$B -m 200 -o $out -f $forest -n $norm $opts -c -t regress.temp  -T 1 -W 10000 -x $viterbi"
(echo $CMD;ls -l $B;uname -a;hostname) > $info
time $CMD > $log  2> $timing
viterbi=$viterbi.restart.1.iteration.1
# | tee $log
echo
echo tail -20 $log
echo
tail -20 $log
echo
echo
lastlog=`ls -t $logbase/log.* | head -2 | tail -1`
lastout=`ls -t $logbase/outparam.* | head -2 | tail -1`
lastviterbi=`ls -t $logbase/viterbi.* | head -2 | tail -1`
lasttiming=`ls -t $logbase/timing.* | head -2 | tail -1`
echo
echo $diff $lastlog $log \| $summarize
echo
$diff $lastlog $log | $summarize
echo
echo $diff $lastout $out \| $summarize
echo
$diff $lastout $out | $summarize
echo
echo $diff $lasttiming $timing \| $summarize
echo
$diff $lasttiming $timing | $summarize
echo $diff $lastviterbi $viterbi \| $summarize
echo
$diff $lastviterbi $viterbi | $summarize
