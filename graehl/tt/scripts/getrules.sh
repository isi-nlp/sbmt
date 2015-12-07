#!/bin/bash
    in=${1:?arg1: prob, forests from forest-em, arg2: rules file, probs:3, counts:4}    
    rules=$2
    probs=$3

    scriptdir=`dirname $0`

 function catz {
     if [ ! -f "$1" ] ; then
         echo "input file $1 not found" 1>&2
         exit -1
    fi
    if [ "${1%.gz}" != "$1" ] ; then
        gunzip -c "$1"
    else
        if [ "${1%.bz2}" != "$1" ] ; then
            bunzip2 -c "$1"
        else
            cat "$1"
        fi
    fi
 }
    
    
    mkdir -p work 2>/dev/null
    out=work/just.`basename $in`.`basename $rules`
    out=${out%.gz}
#    [ -f "$in" ] || exit -1
#    [ -f "$rules" ] || exit -1
        
echo "    catz $in | cut -d ' '  -f 2- > $out.derivtrees"
        catz $in | cut -d ' '  -f 2- > $out.derivtrees
    ids=`tr '()\n' '  ' < $out.derivtrees`          
    echo $in
    echo $out.rules

#    echo looking for ruleids: $ids
    if [ -f "$probs" ] ; then           
            echo             "catz $rules | getlines_id.pl -f iter_prob -a $probs -k [0-9] $ids > $out.rules"
            catz $rules | getlines_id.pl -f iter_prob -a $probs -k [0-9] $ids > $out.rules
    else
            catz $rules | getlines_id.pl $ids > $out.rules
    fi
