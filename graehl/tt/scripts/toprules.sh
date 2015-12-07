#!/bin/bash
f=${1?Give rules list as first argument}
n=${2:-100}

d=`dirname $0`
i=`$d/numberlines.pl | sort -rn | head -$n | cut -f2 -d#`
echo $i
$d/getlines.pl $i $f
