#!/bin/bash

src=.
tgt=/home/wwang/sbmt-dev/sbmt/sbmt_decoder/trunk/3rdparty/lw
for i in $(find . | grep -v "svn"  | grep -v "PerlLib"); do 
    if [ -f $i ]; then
	cp $src/$i $tgt/$i;
    fi
    if [ -d $i ]; then
	mkdir -p $tgt/$i;
    fi
done


