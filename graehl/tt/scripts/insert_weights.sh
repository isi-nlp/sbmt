#!/bin/bash
    . ~graehl/isd/env.sh
weights=${1:?"usage - bunzip2 -c rules.text.file.bz2 | insert_weights.sh weights_file fieldname [num-params] | gzip -c > rules.fieldname.gz"}
    fieldname=${2:-emprob}
    numparams=${3:-160000000}
    
    forest-em -i 0 -p $weights -F $fieldname -B - -b - -I $numparams -m 4 -M 4 -o /dev/null
