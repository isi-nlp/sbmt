#!/bin/sh
#PBS -l arch=x86_64
#PBS -l walltime=168:0:0
#PBS -l nodes=1

. $HOME/.bashrc

N=5
#CORPUS=gigaword_bbn_nan.ardevmedium
#CORPUS=zhbitext
#CORPUS=giga-afp-xin
CORPUS=zh-bi_0.5_i_giga-afp-xin_0.5

#LM=/home/nlg-01/hpc-22/eval06/data/v2.5/LMs/lm.gigaword_afp_xin_only.nodigits.5grams
#LM=/home/nlg-02/data07/eng/v1/LMs/lm.$CORPUS.mttok.lc.nodigits.5gram
#LM=/home/nlg-01/chiangd/kneserney/arbitext.arpa
LM=/home/nlg-02/data07/chi-eng/v3.1/LMs/lm.bi_0.5_i_giga-afp-xin_0.5.mttok.lc.nodigits.5gram

K=16

TRAIN=train

cd $PBS_O_WORKDIR
./quantize.py $LM -n $N $TRAIN -p 16 -b 16 > $CORPUS.p4b4.quantizer
stage/make_biglm $LM --mph-only -o $CORPUS.mph

stage/make_biglm $LM -m $CORPUS.mph -q $CORPUS.p4b4.quantizer -k $K -o $CORPUS.c${K}p4b4.biglm
