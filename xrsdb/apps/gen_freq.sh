TEMP=$TMPDIR

echo "grab lines i from original grammar satisfying i % $m == $k and generate frequency statistics for them"

echo "gunzip -c < $g | $b/nth -k $k -m $m > $TEMP/grammar.$k"
time  gunzip -c < $g | $b/nth -k $k -m $m > $TEMP/grammar.$k

echo "$b/xrsdb_genfreq -f $TEMP/freq.$k < $TEMP/grammar.$k"
time  $b/xrsdb_genfreq -f $TEMP/freq.$k < $TEMP/grammar.$k

mkdir -p $t

echo "cp $TEMP/freq.$k $t/freq.$k"
time  cp $TEMP/freq.$k $t/freq.$k

echo "gzip < $TEMP/grammar.$k > $TEMP/grammar.$k.gz"
time  gzip < $TEMP/grammar.$k > $TEMP/grammar.$k.gz

echo "cp $TEMP/grammar.$k.gz $t/grammar.$k.gz"
time  cp $TEMP/grammar.$k.gz $t/grammar.$k.gz

