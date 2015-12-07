#include "prob.h"
#include "model2.h"
#include <math.h>

extern JmpHash jmphashProb;
extern int rmC[MAXNTS], unk[MAXVOC];

double hashedComputePriorProb(int M, int t, int w)
{ 
  unsigned char fkey[SLEN];
  double prob;
  Key k;
  int pflag, nf=0;
  
  fkey[nf++] = M; fkey[nf++] = t; nf = writeWord(fkey, w, nf); fkey[nf++] = 61;
  k.key = fkey; k.len = nf;

  prob = findProbEffHash(&k, &effhashProb, &pflag);
  if( !pflag || !HPROBS ){
    prob = computePriorProb(M, t, w);
    if( !prob )
      prob = MININF;
    addProbEffHash(&k, &effhashProb, prob);
  }
  return prob;
}

double hashedComputeHeadProb(int H, int P, int t, int w)
{
  unsigned char fkey[SLEN];
  double prob;
  Key k;
  int pflag, nf=0;
  
  fkey[nf++] = H; fkey[nf++] = P; fkey[nf++] = t; nf = writeWord(fkey, w, nf); fkey[nf++] = 1;
  k.key = fkey; k.len = nf;

  prob = findProbEffHash(&k, &effhashProb, &pflag);
  if( !pflag || !HPROBS ){
    prob = computeHeadProb(H, P, t, w, 1);
    if( !prob )
      prob = MININF;
    addProbEffHash(&k, &effhashProb, prob);
  }
  
  return prob;
}

double hashedComputeFrameProb(int subcat, int P, int H, int t, int w, int side)
{
  unsigned char fkey[SLEN], type;
  double prob;
  Key k;
  int pflag, nf=0;

  if( side==1 ) type = 11; 
  else type = 12;

  nf = writeFrame(fkey, subcat, nf); fkey[nf++] = P; fkey[nf++] = H; fkey[nf++] = t; nf = writeWord(fkey, w, nf); fkey[nf++] = type;
  k.key = fkey; k.len = nf;
  prob = findProbEffHash(&k, &effhashProb, &pflag);
  if( !pflag || !HPROBS ){
    prob = computeFrameProb(subcat, P, H, t, w, side, 1);
    addProbEffHash(&k, &effhashProb, prob);
  }

  return prob;
}

double hashedComputeMod1Prob(int M, int mt, int P, int H, int t, int w, int C, int V, int T, int cc, int punc, int side)
{
  unsigned char fkey[SLEN], type;
  double prob;
  Key k;
  int pflag, nf=0;

  if( side==1 ) type = 21;
  else type = 22;

  fkey[nf++] = M; fkey[nf++] = mt; 
  fkey[nf++] = P; fkey[nf++] = H; fkey[nf++] = t; nf = writeWord(fkey, w, nf);
  nf = writeFrame(fkey, C, nf); fkey[nf++] = V; fkey[nf++] = T ; 
  fkey[nf++] = cc; fkey[nf++] = punc; fkey[nf++] = type;
  k.key = fkey; k.len = nf;
  
  prob = findProbEffHash(&k, &effhashProb, &pflag);
  if( !pflag || !HPROBS ){
    prob = computeMod1Prob(M, mt, P, H, t, w, C, V, T, cc, punc, side, 1);
    addProbEffHash(&k, &effhashProb, prob);
  }
  return prob;
}

double hashedComputeMod2Prob(int mw, int M, int mt, int P, int H, int t, int w, int C, int V, int T, int cc, int punc, int side)
{
  unsigned char fkey[SLEN], type;
  double prob;
  Key k;
  int pflag, nf=0;

  if( side==1 ) type = 31;
  else type = 32;

  nf = writeWord(fkey, mw, nf); fkey[nf++] = M; fkey[nf++] = mt; 
  fkey[nf++] = P; fkey[nf++] = H; fkey[nf++] = t; nf = writeWord(fkey, w, nf);
  nf = writeFrame(fkey, C, nf); fkey[nf++] = V; fkey[nf++] = T; 
  fkey[nf++] = cc; fkey[nf++] = punc; fkey[nf++] = type;
  k.key = fkey; k.len = nf;

  prob = findProbEffHash(&k, &effhashProb, &pflag);
  if( !pflag || !HPROBS ){
    prob = computeMod2Prob(mw, M, mt, P, H, t, w, C, V, T, cc, punc, side, 1);
    addProbEffHash(&k, &effhashProb, prob);
  }
    
  return prob;
}

double hashedComputeCP1Prob(int t, int P, int H1, int t1, int w1, int H2, int t2, int w2, int type)
{
  unsigned char fkey[SLEN];
  double prob;
  Key k;
  int pflag, nf=0;

  fkey[nf++] = t; fkey[nf++] = P; 
  fkey[nf++] = H1; fkey[nf++] = t1; nf = writeWord(fkey, w1, nf);
  fkey[nf++] = H2; fkey[nf++] = t2; nf = writeWord(fkey, w2, nf); fkey[nf++] = type;
  k.key = fkey; k.len = nf;

  prob = findProbEffHash(&k, &effhashProb, &pflag);
  if( !pflag || !HPROBS ){
    prob = computeCP1Prob(t, P, H1, t1, w1, H2, t2, w2, type, 1);
    addProbEffHash(&k, &effhashProb, prob);
  }
  
  return prob;
}

double hashedComputeCP2Prob(int w, int t, int P, int H1, int t1, int w1, int H2, int t2, int w2, int type)
{
  unsigned char fkey[SLEN];
  double prob;
  Key k;
  int pflag, nf=0;

  nf = writeWord(fkey, w, nf); fkey[nf++] = t; fkey[nf++] = P; 
  fkey[nf++] = H1; fkey[nf++] = t1; nf = writeWord(fkey, w1, nf); 
  fkey[nf++] = H2; fkey[nf++] = t2; nf = writeWord(fkey, w2, nf); fkey[nf++] = type;
  k.key = fkey; k.len = nf;

  prob = findProbEffHash(&k, &effhashProb, &pflag);
  if( !pflag || !HPROBS ){
    prob = computeCP2Prob(w, t, P, H1, t1, w1, H2, t2, w2, type, 1);
    addProbEffHash(&k, &effhashProb, prob);
  }
  
  return prob;
}
