TEMP=$TMPDIR

echo "assign keywords segment $k of $m in $t, and apply split"

echo "gunzip -c < $t/grammar.$k.gz | $b/xrsdb_assignkeys -f $t/freq > $TEMP/grammar.keyword.$k"
time  gunzip -c < $t/grammar.$k.gz | $b/xrsdb_assignkeys -f $t/freq > $TEMP/grammar.keyword.$k

echo "$b/xrsdb_split -m $m -f $t/freq -p $TEMP/grammar.keyword.$k. < $TEMP/grammar.keyword.$k"
time  $b/xrsdb_split -m $m -f $t/freq -p $TEMP/grammar.keyword.$k. < $TEMP/grammar.keyword.$k

for j in `seq 0 $[$m - 1]`; do
    echo "gzip < $TEMP/grammar.keyword.$k.$j > $TEMP/grammar.keyword.$k.$j.gz"
    time  gzip < $TEMP/grammar.keyword.$k.$j > $TEMP/grammar.keyword.$k.$j.gz
done

for j in `seq 0 $[$m - 1]`; do
    echo "cp $TEMP/grammar.keyword.$k.$j.gz $t/grammar.keyword.$k.$j.gz"
    time  cp $TEMP/grammar.keyword.$k.$j.gz $t/grammar.keyword.$k.$j.gz
done

