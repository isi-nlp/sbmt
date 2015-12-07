#!/bin/bash
scriptdir=`dirname $0`
    in=${1:?  in_derivs:1  rules_pruned:2  expandargs:3 e.g. -q treevizargs:4 e.g. -s treevizprelude:5 e.g. edge[weight=.5]}
    rules_pruned=$2
    expandargs=$3 #e.g. -q
    treevizargs=${4:-''} #e.g. -s
        treevizprelude=${5:-'graph[rankdir=TB,size="16.5,10.5",ratio=compress,mclimit=10];'}
        n=3
        probfield=iter_prob}
echo $treevizargs
            echo $treevizprelude
    mkdir -p work 2>/dev/null
    out=work/`basename "$in"`.$expandargs.$treevizargs.
    [ -f $in ] && [ -f $rules_pruned ] ||  exit -1
        echo using in $in
        echo using rules_pruned $rules_pruned
    if grep -q $probfield "$rules_pruned" ; then
        echo using $probfield regexp
        probargs=" --probregexp $probfield=(\S+)"
    fi
    cut -d ' '  -f 2- $in > $out.derivtrees
        echo $out.derivcaptions 
    perl -n -e '++$n;($prob)=split;if ($prob =~ /^(\d+)\:(.*)/) { $n=$1;$prob=$2; } print "'$in' deriv #$n prob=$prob\n"' $in > $out.derivcaptions || exit

#        perl -e '            use utf8;use HTML::Entities;    binmode STDIN,":gb2312";while (<>) {    print encode_entities($_);    }' < $rules_pruned > $rules_pruned.entities

        echo $out.humanderivtrees 
echo    $scriptdir/expand_ruleids.pl $expandargs $probargs -charset gb2312 -rules $rules_pruned $out.derivtrees
    $scriptdir/expand_ruleids.pl $expandargs $probargs -charset gb2312 -rules $rules_pruned $out.derivtrees > $out.humanderivtrees || exit
    
        #
        #utf8 rightarrow

        #perl -p -e 'use utf8;s| -> |Å‚Üí\\n|g' < $out.humanderivtrees >        $out.humanderivtrees2; mv $out.humanderivtrees2 $out.humanderivtrees
    
echo $out.dot
    treeviz $treevizargs -p "node [shape=box]; edge [fontname=SimSun];node [fontsize=12,fontname=SimSun];graph [fontsize=12,ranksep=.2,fontname=SimSun]; $treevizprelude" -c $out.derivcaptions -i $out.humanderivtrees -o $out.dot || exit
    outpre=$out.dots
    cat $out.dot | $scriptdir/split_dot.pl -o $outpre
#if [ $ARCH != cygwin ] ; then
export DOTFONTPATH=$scriptdir
#fi    
for f in $outpre.*; do    
    n=${f#$outpre.}
    dotout=$in.$expandargs.$n
            echo $f $n $dotout
# dot -Tps $f -o $out.ps
echo dot -Tpng $f -o $dotout.png
             dot -Tpng $f -o $dotout.png
echo dot -Tsvg $f -o $dotout.svg
             dot -Tsvg $f -o $dotout.svg
done    
