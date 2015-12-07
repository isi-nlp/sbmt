#ifndef PROB_H_inc
#define PROB_H_inc

#include "jmphash.h"

#define HPROBS 1 

#define MININF -3000

double hashedComputePriorProb(int M, int t, int w); // label, tag, word

double hashedComputeHeadProb(int H, int P, int t, int w);
double hashedComputeFrameProb(int subcat, int P, int H, int t, int w, int side);
double hashedComputeMod1Prob(int M, int mt, int P, int H, int t, int w, int C, int V, int T, int cc, int punc, int side);
double hashedComputeMod2Prob(int mw, int M, int mt, int P, int H, int t, int w, int C, int V, int T, int cc, int punc, int side);
double hashedComputeCP1Prob(int t, int P, int H1, int t1, int w1, int H2, int t2, int w2, int type);
double hashedComputeCP2Prob(int w, int t, int P, int H1, int t1, int w1, int H2, int t2, int w2, int type);

#endif
