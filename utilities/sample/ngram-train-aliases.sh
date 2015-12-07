function srilm_train {
local text=${1:?srilm_train text output opts...}
shift
local out=$1
shift
ngram-count -sort -text $text -lm $out $*
}

function lwlm_from_srilm {
local srilm=${1:?lwlm_from_srilm srilm output opts...}
shift
local out=$1
shift
LangModel -lm-in $srilm -lm-out $out -trie2sa $*
}
