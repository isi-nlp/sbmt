#!/bin/bash
lmquiet=1
(
if [ "$debugngram" ] ; then
 lmquiet=1
 show=1
 lm=1
 ngram=$debugngram
 lwargs=${lwargs:-"--mismatch --debug 10 --raw-sri -"}
fi
. ~/blobs/bashlib/latest/bashlib.sh
getrealprog
. $d/make.lm.sh
#set -e
#set -x
OLDPATH=$PATH
if [ "$version" ] ; then
 [ "$ON64" ] && archsub=${archsub:-/x86_64/bin}
 versiondir=$BLOBS/mini_decoder/$version$archsub
 export PATH=$versiondir:$PATH
 showvars version versiondir
 require_dirs $versiondir
fi
ngram=${ngram:-3}
decoder=${decoder:-xrs_info_decoder}
base=${base:-small}
xrs=${xrs:-$base.xrs}
outbase=${outbase:-$base}
[ -f $xrs.gz ] && xrs=$xrs.gz
showvars_required base
#showvars_files xrs
mkdir -p $HOST
if [ -z "$nogz" ] ; then
GZ=.gz
fi
foreign=${foreign:-$base.foreign}
outbase=$HOST/$outbase
newxrs=${usenewxrs:-$outbase.new-xrs$GZ}
newxrsfiltered=$outbase.filtered-xrs$GZ
newbrf=$outbase.brf$GZ
newbrfunk=$outbase.uwr$GZ
newarch=$outbase.gar$GZ
if [ $base = oom ] ; then
 if [ "$n" ] ; then
  foreign=$foreign.$n
  perl -e 'print "<seg>";print "a " for (1..'$n'); print "</seg>\n"' > $foreign
  cat $foreign
 fi
fi
[ "$noglue" ] || gluearg="--use-glue 1"
[ "$noxml" ] && plainarg="--plain"
[ "$nostart" ] && nosarg="--intro 0"
[ "$enc" ] && encarg="--encoding $enc"
# "$nounk$"
byline=$outbase.translated.byline
splitforeign=$outbase.split.foreign
consumedforeign=$outbase.consumed.foreign
lmfile=${lmfile:-$base.LW}
showvars_required decoder newxrs newbrf newarch
showvars_optional nounk lm unkargs lmfile textbrf noprep archive_only justprep show closed noarchive noxml noglue nostart ngram instruct
#set -x
prior=tag.prior
if [ -f "$prior" ] ; then
 bonuscount=${bonuscount:-100}
 priorarg="--prior-file $prior --prior-bonus-count $bonuscount"
fi
if [ "$lm" ] ; then
 alm=$lmfile
 nlm=`echo $lmfile | perl -pe 's/\.LW/.'$ngram'.LW/'`
 [ -f "$nlm" ] || nlm=$lmfile.$ngram
 [ -f "$nlm" ] || nlm=`echo $lmfile | perl -pe 's/\d+(\D*)$/'$ngram'$1/'`
 [ -f "$nlm" ] && alm=$nlm
if [ "$nodigit" ] ; then
digit=""
else
digit="@"
fi
if [ "$closed" ] ; then
alm=$alm.closed
occhar=c
else
occhar=o
nounkarg=${nounkarg:-"--add-lm-unk 0"}
openarg="--open 1 $nounkarg"
fi
  [ -f "$alm" ] || alm=$lmfile
 lmfile=$alm
 [ "$forcelmfile" ] && lmfile=$forcelmfile
 showvars_required lmfile
 require_files $lmfile
lmopt=$occhar$digit
compopt=$occhar
topopt=$digit
lmfile2=${lmfile2:-$lmfile}
if [ "$biglm" ] ; then
 lmarg="--dynamic-lm-ngram big[$occhar][$biglm]"
else
if [ "$staticlm" ] ; then
 lmarg="--lm-ngram $lmfile"
else
 if [ "$multilm" ] ; then
  lmarg="--dynamic-lm-ngram multi=multi[$topopt][lm=lw[$compopt][$lmfile],lm2=lw[$compopt][$lmfile2]] --weight-string lm1:.3,lm2:.7,lm1-unk:6,lm2-unk:14"
 else
  lmarg="--dynamic-lm-ngram lw[$lmopt][$lmfile]"
 fi
fi
fi
showvars lmarg
 [ "$ngram" ] && ngo="--ngram-order $ngram"
 [ "$lngram" ] && ngo="--ngram-order $lngram"
 [ "$lngram" ] && hngo="--higher $ngram"
fi

[ "$textbrf" ] && noarchive=1
[ "$align" ] && alignfully="--align-fully 1"
if [ -z "$noprep" ] ; then
 require_files $foreign
 split-byline.pl $encarg $nosarg $plainarg -t $byline -u $splitforeign -c $consumedforeign $foreign
 require_files $byline $splitforeign $consumedforeign
if [ -z "$archive_only" ] ; then
#set -x
 [ "$usenewxrs" ] || new_decoder_weight_format $nosarg $gluearg -i $xrs -o $newxrs $alignfully
 itg_binarizer --filter 1 -i $newxrs > $newxrsfiltered
 itg_binarizer -i $newxrsfiltered -o $newbrf
 if [ "$nounk" ] ; then
  newbrfunk=$newbrf
 else
  unknown_word_rules $unkargs --foreign-file $splitforeign --rule-file $newbrf --copy true --output $newbrfunk
 fi
fi
showvars_optional noarchive
if [ -z "$noarchive" ] ; then
 archive_grammar -i $newbrfunk -o $newarch
fi
#[ "$nostart" ] && bnosarg="--nointro"
fi

#showvars_files newxrs newbrf newarch

#set -x
defaultlogbase=$HOST/logs/`filename_from $base $ngo $lmarg $weight $*`
log=${log:-$defaultlogbase.log}
err=${err:-$defaultlogbase.err}
output=${output:-$defaultlogbase.output}
mkdir -p `dirname $log`
mkdir -p `dirname $output`

if [ -z "$oldformat" ] ; then
 format="--show-spans 0"
fi
if [ -z "$justprep" ] ; then
 weight=${weights:-$base.weights}
 logfile=${logfile:-$log}
 nbests=${nbests:-100}
 showvars_files foreign weight
 weightarg="--weight-file $weight"
 showvars_required output nbests
 showvars_required foreign weight output
 brfopt="--grammar-archive $newarch"
 if [ "$textbrf" ] ; then
  brfopt="--brf-grammar $newbrfunk"
 fi
 if [ "$instruct" ] ; then
    if [ -f $instruct ] ; then
      ifile=$instruct
    else
        ifile=$outbase.instruct
        make-instruction-file.pl -n $instruct -archive `basename $newarch` < $splitforeign > $ifile
    fi
  instr="--instruction $ifile"
 else
  instr="--foreign $splitforeign"
 fi
# [ "$nolog" ] || logfilearg="--log-file $logfile"
outarg="--output $output $logfilearg"
[ "$per" ] && perarg="--per $per"
 justdecoder="$decoder $brfopt $instr $weightarg --nbests $nbests $openarg $hngo $ngo $lmarg $priorarg $format $perarg $*"
 rundecoder="$justdecoder $outarg"
#--log-file $logfile
showvars brfopt weightarg output

echo =============== Running decoder:
echo
echo $justdecoder
echo
echo ===============
 if [ "$gdb" != "" ] ; then
  gdb --args $rundecoder
 else
 set -x
  $gdbcmd $rundecoder 2>&1 | tee $logfile
  filtersum=$outbase.summary
  filterpre=${filterpre:-filter}
  filterbase=$outbase.$filterpre.per=$per
  extract-field.pl -f hyp $output > $filterbase.hyp
#  echo $filterbase >> $filtersum
  wc -l $filterbase.hyp >> $filtersum
#  echo $filterbase unique >> $filtersum
  sort $filterbase.hyp | uniq -c > $filterbase.uniq
  wc -l $filterbase.uniq >> $filtersum
  echo >> $filtersum
#  extract-field.pl -f derivation $output | sort | uniq -c
 set +x
  savelog $logfile
  cp $logfile ~/tmp/lastlog
  savelog $output
  cp $output ~/tmp/lastout
  totalscore.pl -q $weightarg $output
#  sbtm-score.pl  $brfopt $weightarg $output
 fi
if [ "$show" ] ; then
if [ $show = all ] ; then
 cat $output
else
 [ $show = 1 ] && show=20
 head -n $show $output
 echo ---- continued in $output
fi
fi
 if [ "$lm" -a ! "$noverify" ] ; then
  srilm=`SRIfromLW $lmfile`
  [ "$closed" ] && lmbase=srilm.closed
  [ -f $lmbase ] || lmbase=$srilm
  srilm=${srilm:-$lmbase}
    showvars_required srilm
  [ -f $srilm ] && srilmargs="--sri-lm-ngram $srilm"
  lm_sents=${lm_sents:-$defaultlogbase.sents.for.lm}
  lm_output=${lm_output:-$defaultlogbase.lmscore}
  lm_verify=${lm_verify:-$defaultlogbase.lmverify}
  outarg="--score-output $lm_output"
#  dbgopt="--debug 2"
 [ ! "$closed" ] && lwver="--lm-ngram $lmfile" && openarg="--open-class-lm"
 [ "$lmattr" ] && lwver="--attr-lm-cost $lmattr $lwver"
 [ "$lmquiet" ] && lmquiet="--quiet"
 [ "$nolw" ] && lwargs=""
 [ "$nosri" ] && srilmargs=""
  lw-lm-score.pl $lwargs $srilmargs $lmquiet $lwver --sentence-output $lm_sents $ngo $openarg $dbgopt $outarg $output 2>&1 | tee $lm_verify
  savelog $lm_verify
  savelog $lm_sents
  savelog $lm_output
#  cat $lm_output
 fi
 showvars_optional lmfile nlm
fi

 echo DONE WITH:
 echo $justdecoder
 echo
 echo .

tail -n 10 $filtersum
[ "$sbtmverify" ] &&    sbtm-score.pl  $brfopt $weightarg $output
)
which mini_decoder
