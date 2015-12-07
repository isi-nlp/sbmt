#!/bin/bash
## usage: out=output.tiburon.rules rules=input.tiburon.rules sents=training.sentpairs tiburon-em-workflow.sh 
## optional variables: logprob=1 dryrun=1 START=1 END=6 STEP3="--args='--nbests=10'" work=workdir/fileprefix clean=1 zerobelow=1e-30

iter=${iter:-50}
[ "$logprob" ] || ehat="--human-probs"
blib=`which bashlib.sh`
[ "$blib" ] || blib=~/blobs/bashlib/latest/bashlib.sh
. $blib
getrealprog
export PATH=$d:$PATH
work=${work:-work/`basename $rules`}
restarts=${restarts:-4}
zerobelow=${zerobelow:-0}
showvars_required rules sents out work restarts zerobelow iter
mkdir -p `dirname $work`
[ "$out" ] && outarg="-o $out"
require_files $rules $sents

[ "$clean" ] && rm -f  $work.*

mkdir -p `dirname $out`

Xrs=$work.xrs
Forceout=$work.force
Norms=$work.norms
Id=$work.id
Weight=$work.weights.gz
Forest_ttable=$work.forest.ttable.gz
Forests=$Forceout.forests.gz
Tied_forests=$work.tied.forests

START=${START:-0}
END=${END:-999999}

function step 
{
    [ $1 -ge $START -a $1 -le $END ] 
}

function fail
{
echo FAILED: $*
FAIL=echo
CHECK=true
}

function need_files
{
set +x
local f
for f in $*; do
 [ -f $f ] || fail  missing required file $f
done
set -x
}

FAIL=time
CHECK=
if [ "$dryrun" ] ; then
 FAIL=echo
 CHECK=true
 function need_files
 {
   return 0
 }
else
 set -x
fi
noexit=0

rules_orig=$rules
    if [ "$restarts" = 0 ] ; then
     rules=$work.rules.random
     rinit=$work.rule.init.params
     rarg="-N -I $rinit -r 0"
    else
     rinit=
     rarg="-r $restarts"
    fi

if step 0; then
if [ "$rinit" ] ; then
     $CHECK need_files $rules_orig
     $FAIL tiburon-randomize.pl -initparam $rinit -out $rules $rules_orig
     $CHECK need_files $rinit $rules
fi
fi
if step 1; then
    $CHECK need_files $rules
    $FAIL tiburon-to-xrs.pl -in $rules -o $Xrs $STEP1 || fail
    $CHECK need_files $Xrs
fi
if step 2; then
    $CHECK need_files $Xrs
    $FAIL xrs-em-prep.pl -param-id $Id -norm $Norms -forest-trans $Forest_ttable $Xrs $STEP2 || fail
    $CHECK need_files $Id $Norms $sents
fi
if step 3; then
    $CHECK need_files $Xrs $sents
    $FAIL force-tree-ins.pl -strip-xrs-state -decode -xrs $Xrs -out $Forceout $sents $STEP3 || fail
    $CHECK need_files $Forests
fi
if step 4; then
    $CHECK need_files $Forests $Forest_ttable
    $FAIL translate-forest-ids.pl $Forests -forest-trans $Forest_ttable -o $Tied_forests $STEP4 || fail
    $CHECK need_files $Tied_forests
fi
if step 5; then
    $CHECK need_files $Tied_forests $Norms $Id
    $FAIL forest-em -e 0 -i $iter -z -X 0 -U $rarg -H -b $Id -n $Norms -f $Tied_forests -B $Weight $STEP5 || fail
    $CHEC need_files $Weight
fi
if step 6; then
    $CHECK need_files $Weight $rules
    $FAIL weights-to-tiburon.pl -zero-below=$zerobelow $ehat -w $Weight $outarg $rules $STEP6 || fail
    [ "$out" ] && $CHECK need_files $out
fi
if [ "$rinit" -a "$tibcompare" ] ; then
export PATH=.:$PATH
TIB=tiburon
  echo2 comparing with Tiburon training using same random init:
$TIB -no zzz -t $iter $sents $rules 2>&1 | tee $work.tiburon.em.log
cat zzz | grep -v '# 0.0$' | grep -v '#.*E-' >$base.sp.xrs.t10
$TIB -c $base.sp.xrs.t10
cp $base.sp.xrs.t10 $work.t10
(
for i in NN NNS JJ DT VBD IN NPB
do
  sort-rules $base.sp.xrs.t10 $i
done
)  | tee $work.tiburon.em.sorted
cp $work.tiburon.em.sorted $base.sp.t10.by.tiburon

fi

set +x
echo2
echo2 DONE.
echo2
echo2 INPUT:
showvars_required rules sents
echo2
echo2 INTERMEDIARY:
showvars_required Xrs Norms Id Weight Forest_ttable Forests Tied_forests
echo2
echo2 OUTPUT:
showvars_required out
