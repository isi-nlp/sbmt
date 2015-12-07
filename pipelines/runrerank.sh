#!/usr/bin/env bash
#
# convenience wrapper for reranking
# pass in the arguments to runrerank and the corpora for applyrerank
# this sets up the chained hpc calls
# assumption: you have generated n-best lists (for n of ~ 1000, best to have minimal repeats) for 
# training and all eval corpora, you have adjoined them with reranking features (which you name with -f)
# arguments with more than one token, e.g. -p, -f should be quoted.
# filenames need absolute paths.

# TODO: be able to parse pipeline yaml files to make this easier to run.


# example: you have an nplm feature you want to rerank with. Tuning without the feature was best in iteration 44; you have weights.44 from mira. 
# You have generated 1k-best lists for each of tune, test, sct sets, called tune.nbest. You adjoin each line with the field "nplm=<featval>" forming tune, test, sct.nbest.adjoin
# where <featval> matches the sign of other similar features.

# you have lc tok references tune.ref.0 tune.ref.1 for tune. You want everything in the (extant) directory "outdir".

# (in the below you use absolute references to file names)
# you run runrerank.sh -f nplm -t tune.nbest.adjoin -w weights.44 -r "tune.ref.0 tune.ref.1" -o outdir test.nbest.adjoin sct.nbest.adjoin



CURRDIR=$(dirname $(readlink -f $0))
SCRIPTDIR=$CURRDIR/scripts
RERANKER=$SCRIPTDIR/runrerank.py
APPLIER=$SCRIPTDIR/applyrerank.py
QSUBRUN=$CURRDIR/util/qsubrun

# TODO: replace this with something working!
OLDFEATS="text-length derivation-size lm1 lm2" # O
NEWFEATS= # f
TUNECORPUS= # t
TUNEWEIGHTS= # w
TUNEREFS= # r
TUNEPARAMS= # P
OUTDIR=$PWD # o
OUTSUFFIX="onebest.rerank" # S
while getopts “O:f:t:w:r:P:o:S:” OPTION
do
     case $OPTION in
         O)
             OLDFEATS=$OPTARG
             ;;
         f)
             NEWFEATS=$OPTARG
             ;;
         t)
             TUNECORPUS=$OPTARG
             ;;
         w)
             TUNEWEIGHTS=$OPTARG
             ;;
         r)
             TUNEREFS=$OPTARG
             ;;
         P)
             TUNEPARAMS=$OPTARG
             ;;
         o)
             OUTDIR=$OPTARG
             ;;
         S)
             OUTSUFFIX=$OPTARG
             ;;
         ?)
             echo "Read shell script to learn usage"
             exit
             ;;
     esac
done
shift $(( OPTIND - 1 ))

if [[ -z $TUNEREFS ]] || [[ -z $NEWFEATS ]] || [[ -z $TUNECORPUS ]] || [[ -z $TUNEWEIGHTS ]]
then
     echo "Mandatory arguments are r, f, t, w"
     exit 1
fi

TUNEFEATS="$OLDFEATS $NEWFEATS"
RERANKWEIGHTS="$OUTDIR/rerank.weights"
tcbasename=`basename $TUNECORPUS`;
TUNERERANK="$OUTDIR/$tcbasename.$OUTSUFFIX"

JOBSTR="$QSUBRUN $RERANKER -i $TUNECORPUS -f $TUNEFEATS -w $TUNEWEIGHTS -r $TUNEREFS $TUNEPARAMS -o $RERANKWEIGHTS -b $TUNERERANK";
#echo $JOBSTR
TUNEJOB=`$JOBSTR`;

for corpus in "$@"; do
    $QSUBRUN -W depend=afterok:$TUNEJOB -- $APPLIER -i $corpus -w $TUNEWEIGHTS -k $RERANKWEIGHTS -b $corpus.$OUTSUFFIX;
done;