#!/usr/bin/env bash
SDIR=$(cd $(dirname $0); pwd)

IN=$1
OUT=$2
P=${TMPDIR-/tmp}
T=$(mktemp -d $P/XXXXXX)
echo "logs in $T"
cat $IN | $SDIR/collinsmapper -p ruleinfo > $T/ruleinfo
cat $T/ruleinfo | $SDIR/collinsmapper -p dictionary | cut -f1 | sort -u | $SDIR/mkcollinslm -f $OUT -p dictionary
echo dictionary loaded
$SDIR/mkcollinslm -f $OUT -p collinslm
cat $T/ruleinfo | $SDIR/mkcollinslm -f $OUT -p rulemodel 2> $T/events.rulemodel
echo rule model loaded
cat $T/ruleinfo | $SDIR/mkcollinslm -f $OUT -p deptagmodel 2> $T/events.deptagmodel
echo deptag model loaded
cat $T/ruleinfo | $SDIR/mkcollinslm -f $OUT -p depwordmodel 2> $T/events.depwordmodel
echo depword model loaded
$SDIR/mkcollinslm -f $OUT -p complete

