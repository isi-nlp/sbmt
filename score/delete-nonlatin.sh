#!/bin/sh

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/nlg-01/blobs/icu/latest/lib
export LD_LIBRARY_PATH

PATH=/home/nlg-01/blobs/icu/latest/bin:$PATH
export PATH

SCRIPTDIR=`dirname $0`;

#uconv -f utf8 -t utf8 --callback escape-xml | /home/nlg-03/dmarcu/NIST-EVAL06/postprocess/delete-nonlatin.pl
uconv -f utf8 -t utf8 --callback escape-xml | $SCRIPTDIR/delete-nonlatin.pl
