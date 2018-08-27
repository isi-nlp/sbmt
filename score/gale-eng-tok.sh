#!/usr/bin/env bash
THOME=$(dirname $BASH_SOURCE)
cat $1 | perl $THOME/lw_tokenize.pl -conf $THOME/lw_tokenize_eng_ptb.conf.v2 | perl $THOME/lw_tokenize.pl -conf $THOME/lw_tokenize_eng_ptb2mt.conf
