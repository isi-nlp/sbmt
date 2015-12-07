#!/bin/sh

ROOT=$(dirname $0)
#IN=$1
#OUT=$2

$ROOT/rule2tree.pl | python $ROOT/single-level-rewrite.py
