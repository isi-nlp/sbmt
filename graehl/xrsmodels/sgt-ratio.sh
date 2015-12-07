perl -pe 's/\#\d+\b//g' deriv > deriv.nopointers
count-id-freq.static -f freqs.f -b freqs.bins -c freqs.counts -i deriv.nopointers
perl -ane '$s=$F[1]/$F[0];print "$F[0] $s\n"' < freqs.sgt > freqs.sgt.ratio

#w=last;grep -n '^ 1$' $w.counts | cut -d: -f1 > $w.1count.lines
#sample_lines.pl -n 315384 deriv.nopointers > first.11th &
#getlines.pl -lines last.1count.lines < first.counts > first.counts.of.last1       
#perl -ne '$s+=$_;END{print "$s\n"}' first.counts.of.last1    
#getlines.pl -nosort -lines last.1count.lines < first.counts > first.counts.of.last1

~/projects/xrsmodels$ grep sgt *.sh
4.make.sh:$sparsebin -sparse $sgtratio -o $out -keyfield $fieldname -valfield $sgtfieldname -prob)  2>$err
common.sh:sgtdir=~/p/smoothing/$lang
common.sh:sgtfieldname=sent_sgt_ratio
common.sh:sgtratio=$sgtdir/freqs.sgt.ratio
common.sh:intcounts=$sgtdir/freqs.counts
fix.final.sh:$sparsebin -sparse $sgtratio -o $safeout/rules.gz -keyfield $fieldname -valfield $sgtfieldname -prob $safeout/rules.3.gz
~/projects/xrsmodels$ grep sparse *.sh
4.make.sh:$sparsebin -sparse $sgtratio -o $out -keyfield $fieldname -valfield $sgtfieldname -prob)  2>$err
common.sh:sparsebin=addfield-sparse.pl
fix.final.sh:$sparsebin -sparse $sgtratio -o $safeout/rules.gz -keyfield $fieldname -valfield $sgtfieldname -prob $safeout/rules.3.gz
