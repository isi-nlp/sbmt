# export PATH=$BLOBS/mini_decoder/unstable:$PATH
function srilm_train {
local text=${1:?srilm_train text output opts...}
shift
local out=$1
shift
local args=''
if [ ! "$*" ] ; then
 args="-tolower "
fi
ngram=${ngram:-5}
echo using ngram order $ngram
local unkargs="-unk"
local ngoargs="-order $ngram"
ngram-count $ngoargs $unkargs $args -sort -text $text -lm $out $*
ngram-count $ngoargs $args -sort -text $text -lm $out.closed $*
}

function lwlm_from_srilm {
local srilm=${1:?lwlm_from_srilm srilm output opts...}
shift
local out=$1
shift || true
[ "$out" ] || out=`LWfromSRI $srilm`
LangModel -lm-in $srilm -lm-out $out -trie2sa "$@"
echo "Done.  sri $srilm => lw $out"
}

function SRIfromLW {
echo $* | perl -pe 's/lwlm/srilm/g or $_.=".srilm"'
}

function LWfromSRI {
echo $* | perl -pe 's/srilm/lwlm/g or $_.=".lwlm"'
}

function bigfromSRI {
echo $* | perl -pe 's/srilm/biglm/g or $_.=".lwlm"'
}

function sri_order {
    perl -e '$N=0;while(<>) { last if (/^\\1-grams:$/); $N=$1 if /^ngram (\d+)=(\d+)$/; };print "$N\n"' "$@"
}

biglmdef=/home/nlg-03/mt-apps/biglm/20100824
biglm=${biglm:-$biglmdef}
if ! [ -d $biglm ] ; then
    biglm=biglmdef
fi
function biglm_from_srilm {
local ngram=${ngram}
local sri=${1:?biglm_from_srilm srilm [output] [quantize opts...]}
shift
if [ "$ngram" = "" ] ; then
 ngram=`sri_order $sri`
fi
local out=$1
shift || true
[ "$out" ] || out=`bigfromSRI $sri`
local make_biglm="$biglm/pagh/bin/gcc-4.3.3/release/make_biglm"
#/home/nlg-03/mt-apps/biglm/20100824/pagh/bin/gcc-4.3.3/release/make_biglm

set -x
$biglm/tools/quantize.py $sri -n $ngram -u .01 -P 4 -B 4 > $out.quant
$make_biglm $sri --mph-only -o $out.mph
$make_biglm $sri -m $out.mph -q $out.quant -k 16 -o $out
set +x
echo "Done.  sri $srilm => big $out"
}

both_from_srilm() {
    lwlm_from_srilm "$@"
    biglm_from_srilm "$@"
}

function lwlm_train {
local text=${1:?lwlm_train text output opts...}
shift
local out=$1
shift
#local srilm=${text%.training}
#srilm=$srilm.SRI
 local srilm=`SRIfromLW $out`
 srilm_train $text $srilm $*
 lwlm_from_srilm $srilm $out
 lwlm_from_srilm $srilm.closed $out.closed
}

function sentence_split {
 perl -ne 'chomp;split /\b\.\s+/;for (@_) {print lc($_),"\n" unless length($_)<10}' $*
}

#todo: tokenize
function lm_prep
{
 perl -ne 'chomp;s/\d/\@/g;split /\b\.\s+/;for (@_) {print lc($_),"\n" unless length($_)<10}' $*
}

function sub_srilm
{
local usage="USAGE: sub_srilm ngram_order higher_order_srilm output_srilm"
local n=${1:?$usage}
shift
local in=${1:?$usage}
shift
local out=${1:?$usage}
shift
showvars_required n in out
make-sub-lm maxorder=$n $in $* > $out
}

function sub_srilms
{
local usage="USAGE: sub_srilm max_order higher_order_srilm output_srilm_prefix"
local M=${1:?$usage}
shift
local in=${1:?$usage}
shift
local out=${1:?$usage}
shift
for n in `seq 2 $M` ; do
    sub_srilm $n $in $out.$n.SRI
    lwlm_from_srilm $out.$n.SRI
done
}

function lwlms_train {
local text=$1
shift
local out=$1
shift
local maxo=$1
shift
for i in `seq 1 $maxo`; do
 echo $i-gram
 ngram=$i lwlm_train $text $out.$i
done
}
