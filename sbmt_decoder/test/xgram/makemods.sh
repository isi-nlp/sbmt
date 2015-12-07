for i in `seq 0 3`; do 
    $bin/nth -k $i -m 4 < xgram.xrs > xgram.$(($i))m4.xrs;
    $bin/itg_binarizer < xgram.$(($i))m4.xrs > xgram.$(($i))m4.brf; 
done;

for i in `seq 0 3`; do
    for j in `seq $(($i+1)) 3`; do
	cat xgram.$(($i))m4.xrs xgram.$(($j))m4.xrs > xgram.$i$(($j))m4.xrs;
	$bin/itg_binarizer < xgram.$i$(($j))m4.xrs > xgram.$i$(($j))m4.brf;
    done;
done;

for i in `seq 0 3`; do
    for j in `seq $(($i+1)) 3`; do
	for k in `seq $(($j+1)) 3`; do
	    cat xgram.$(($i))m4.xrs xgram.$(($j))m4.xrs xgram.$(($k))m4.xrs > xgram.$i$j$(($k))m4.xrs;
	    $bin/itg_binarizer < xgram.$i$j$(($k))m4.xrs > xgram.$i$j$(($k))m4.brf;
	done;
    done;
done;

$bin/itg_binarizer < xgram.xrs > xgram.brf;
