TEMP=$TMPDIR

echo "create skeletal database and merged freq table in $t from $m pieces"

inargs=""
for k in `seq 0 $[$m - 1]`; do
    inargs="$inargs $t/freq.$k"
done

echo "$b/xrsdb_mergetables $t/freq $inargs"
time  $b/xrsdb_mergetables $t/freq $inargs

echo "$b/xrsdb_create -d $x -f $t/freq"
time  $b/xrsdb_create -d $x -f $t/freq


    
