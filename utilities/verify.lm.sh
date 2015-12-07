#!/bin/bash
. ~graehl/isd/hints/bashlib.sh

if [ "$nonum" ] ; then
 nonum="--nolm-at-numclass"
fi
EPSILON=${EPSILON:-.0005}
lm=${lm:-~/ql/ce.lm.lw}
lmfield=${lmfield:-lmcost}
ef=extract-field.pl
prep=lwlm-prep.pl
diffline=difflines.pl
lwlm=LangModel
in=${1:?first arg: name of nbests containing file}
out=$in.diff
out2=$in.diff.sum
nbest=$in.nbest
hyp=$in.hyp
lmin=$in.hyp.lw
lmcost=$in.lmcost
lwlmcost=$in.lwlmcost
if [ ! "$justdiff" ] ; then
$ef -f hyp -whole $nbest -o $hyp $in
$ef -f $lmfield -o $lmcost --paste '\t' $in
$prep $nonum -o $lmin $hyp
if [ ! "$skiplm" ] ; then
 $lwlm -prob -lm $lm -in $lmin 2>&1 > $lwlmcost.log
fi
perl -ne 'print "$1\n" if /^\(-([^)]+)\)$/' $lwlmcost.log > $lwlmcost
fi
$diffline -compare 'epsilon_equal($a,$b,'$EPSILON') || (log_numbers("LWLM=$a decoder=$b,delta=".abs($a-$b))&&0)' -eval '$_=$1 if /^(\S*)\s+NBEST.*$/;$_=$1 if /^(\S+)Nbest/' $lwlmcost $lmcost -o $out 2>$out2
#head -n 6 $out
head=200 headz $out2
