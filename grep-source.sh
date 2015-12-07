#!/bin/bash
d=`dirname $0`
base=${base:-$d}
path=${path:-.}
if [ "$no_case" ] ; then
 no_case_opt=-i
fi 
cd $base
find $path \( -type d -and -name .svn -and -prune \) -o \(  -name \*.m4 -name configure.ac -o -name configure.in -o  -name Makefile.am -o -name \*.ipp -o -name \*.cpp -o -name \*.h -o -name \*.hpp \) -exec grep -n $grep_opt $no_case_opt -H "$*" {} \;
