TEMP=$TMPDIR

echo "apply crossmerge for piece $k of $m and populate database in $t"

inargs=""

for j in `seq 0 $[$m - 1]`; do
    echo "gunzip -c < $t/grammar.keyword.$j.$k.gz | sort > $TEMP/g.$j"
    time gunzip -c < $t/grammar.keyword.$j.$k.gz | sort > $TEMP/g.$j
    inargs="$inargs $TEMP/g.$j"
done

echo "sort -m $inargs > $TEMP/grammar.keyword.$k"
time  sort -m $inargs > $TEMP/grammar.keyword.$k

echo "$b/xrsdb_populate $x < $TEMP/grammar.keyword.$k"
time  $b/xrsdb_populate $x < $TEMP/grammar.keyword.$k

echo "gzip < $TEMP/grammar.keyword.$k > $t/grammar.keyword.$k.gz"
time gzip < $TEMP/grammar.keyword.$k > $t/grammar.keyword.$k.gz

