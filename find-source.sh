#!/bin/bash
d=`dirname $0`
path=${path:-$d}
find $path \( -name \*.ipp -o -name \*.cpp -o -name \*.hpp \) $*
