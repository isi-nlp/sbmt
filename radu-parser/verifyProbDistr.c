#include "model2.h"

int strChunks, mallocChunksKey, mallocChunksNode, mallocChunksTable, mallocChunksHash;
int allb;

double pthresh = 1;
double epsilonDistr = 0.001;

extern int NLEX, NNTS;

int F[] = {
0,
1,
10,
100,
1000,
10000,
10001,
10010,
1010,
10100,
10110,
10200,
1100,
11000,
20,
200,
2000,
20000,
20100,
21000,
40000
};
int nF = 21;

int getRand1ToN(int N);
int getRandLabel(int flag);
int properLabel(int L);

int main(int argc, char **argv)
{
  char buf[SLEN], events[SLEN], grammar[SLEN];
  int c, nt, i, H, P, t, w, f, vi, tau, cc, punc, M, mt, mw, headSample, frameSample, mod1Sample, mod2Sample, cp1Sample, cp2Sample, unkflag, side, cp, cpw, cnt, sample;
  double prob, cprob, tprob, tlprob, trprob, ntotal;
  struct timeval mytime;

  if( argc!=8 )
    { fprintf(stderr, "Usage: %s PTB|MT headSample frameSample mod1Sample mod2Sample cp1Sample cp2Sample\n", argv[0]); exit(-1); }

  sprintf(events, "../TRAINING/rules.%s", argv[1]); 
  sprintf(grammar, "../GRAMMAR/%s", argv[1]); 

  SBLM = 1;
  loadSBLM(events, grammar);

  headSample = atoi(argv[2]);
  frameSample = atoi(argv[3]);
  mod1Sample = atoi(argv[4]);
  mod2Sample = atoi(argv[5]);
  cp1Sample = atoi(argv[6]);
  cp2Sample = atoi(argv[7]);

  gettimeofday(&mytime,0); 
  /* srand((unsigned int)mytime.tv_sec); */
  srand(1);

  /* head */
  sample = 0;
  while( sample < headSample ){
    P = getRandLabel(1);
    t = getRandLabel(2);
    w = getRand1ToN(NLEX);
    
    prob = 0;
    for(H=1, cnt=0; H<=nlabelIndex; H++)
      if( properLabel(H) ){
	cprob = computeHeadProb(H, P, t, w, 0);
	prob += cprob;
	cnt++;
      }
    if( fabs(1-prob) > epsilonDistr )
      printf("%4d P_h([%d elem]|%s %s %s) = %.10f\n", sample, cnt, labelIndex[P], labelIndex[t], wordIndex[w], prob);

    sample++;
  }
  printf("Head prob distributions verified\n");

  /* frames */
  sample = 0;
  while( sample < frameSample ){
    P = getRandLabel(1);
    H = getRandLabel(0);
    t = getRandLabel(2);
    w = getRand1ToN(NLEX);
 
    /* left */
    prob = 0;
    for(f=0, cnt=0; f<nF; f++){
      cprob = computeFrameProb(F[f], P, H, t, w, 1, 0);
      prob += cprob;
      cnt++;
    }
    if( fabs(1-prob) > epsilonDistr )
      printf("%.5d P_lf([%d elem]|%s %s %s %s) = %.10f\n", sample, cnt, labelIndex[P], labelIndex[H], labelIndex[t], wordIndex[w], prob);

    /* right */
    prob = 0;
    for(f=0, cnt=0; f<nF; f++){
      cprob = computeFrameProb(F[f], P, H, t, w, 2, 0);
      prob += cprob;
      cnt++;
    }
    if( fabs(1-prob) > epsilonDistr )
      printf("%.5d P_rf([%d elem]|%s %s %s %s) = %.10f\n", sample, cnt, labelIndex[P], labelIndex[H], labelIndex[t], wordIndex[w], prob);
  
    sample++;
  }
  printf("Frame prob distributions verified\n");
  
  /* mod1 */
  sample = 0;
  while( sample < mod1Sample ){
    P = getRandLabel(1);
    H = getRandLabel(0);
    t = getRandLabel(2);
    w = getRand1ToN(NLEX);
    
    f = getRand1ToN(22)-1;
    vi = getRand1ToN(2)-1;
    tau = getRand1ToN(2)-1;
    side = getRand1ToN(2);

    prob = 0;
    for(M=0, cnt=0; M<=nlabelIndex; M++)
      if( properLabel(M) )
	for(mt=0; mt<=nlabelIndex; mt++){
	  if( M==0 && mt>0 ) continue;
	  if( M>0 && mt==0 ) continue;
	  if( !isnt[mt] && properLabel(mt) ){
	    if( M==0 ){
	      cprob = computeMod1Prob(M, mt, P, H, t, w, F[f], vi, tau, 0, 0, side, 0);
	      prob += cprob;
	      cnt++;
	      continue;
	    }
	    for(cc=0; cc<=1; cc++)
	      for(punc=0; punc<=1; punc++){
		cprob = computeMod1Prob(M, mt, P, H, t, w, F[f], vi, tau, cc, punc, side, 0);

		prob += cprob;
		cnt++;
	      }
	  }
	}
    if( fabs(1-prob) > epsilonDistr )
      if( side==1 )
	printf("%5d P_lm1([%d elem]|%s %s %s %s %d %d %d) = %.10f\n", sample, cnt, labelIndex[P], labelIndex[H], labelIndex[t], wordIndex[w], F[f], vi, tau, prob);
      else
	printf("%5d P_rm1([%d elem]|%s %s %s %s %d %d %d) = %.10f\n", sample, cnt, labelIndex[P], labelIndex[H], labelIndex[t], wordIndex[w], F[f], vi, tau, prob);
 
    sample++;
  }
  printf("Mod1 prob distributions verified\n");

  /* mod2 */
  sample = 0;
  while( sample < mod2Sample ){
    P = getRandLabel(1);
    H = getRandLabel(0);
    t = getRandLabel(2);
    w = getRand1ToN(NLEX);
    
    f = getRand1ToN(22)-1;
    vi = getRand1ToN(2)-1;
    tau = getRand1ToN(2)-1;
    cc = getRand1ToN(2)-1;
    punc = getRand1ToN(2)-1;
    side = getRand1ToN(2);

    M = getRandLabel(0);
    mt = getRandLabel(2);

    prob = computeMod2Prob(UNKINDEX, M, mt, P, H, t, w, F[f], vi, tau, cc, punc, side, 0);;
    for(mw=1; mw<=nwordIndex; mw++){
      cprob = computeMod2Prob(mw, M, mt, P, H, t, w, F[f], vi, tau, cc, punc, side, 0);
      if( cprob > 1.0/(NLEX+1) && 0 ){
	Debug = 1;
	cprob = computeMod2Prob(mw, M, mt, P, H, t, w, F[f], vi, tau, cc, punc, side, 0);
	Debug = 0;
      }
      prob += cprob;
    }

    if( fabs(1-prob) > epsilonDistr )
      if( side==1 )
	printf("%3d P_lm2(.|%s %s %s %s %s %s %d %d %d %d %d left) = %.10f\n", sample, labelIndex[M], labelIndex[mt], labelIndex[P], labelIndex[H], labelIndex[t], wordIndex[w], F[f], vi, tau, cc, punc, prob);
      else
	printf("%3d P_rm2(.|%s %s %s %s %s %s %d %d %d %d %d right) = %.10f\n", sample, labelIndex[M], labelIndex[mt], labelIndex[P], labelIndex[H], labelIndex[t], wordIndex[w], F[f], vi, tau, cc, punc, prob);
    
    sample++;
  }
  printf("Mod2 prob distributions verified\n");
  
  /* cp1 */
  sample = 0;
  while( sample < cp1Sample ){
    P = getRandLabel(1);
    H = getRandLabel(0);
    t = getRandLabel(2);
    w = getRand1ToN(NLEX);
    M = getRandLabel(0);
    mt = getRandLabel(2);
    mw = getRand1ToN(NLEX);

    prob = 0;
    for(cp=1; cp<=nlabelIndex; cp++)
      if( !isnt[cp] && properLabel(cp) ){
	cprob = computeCP1Prob(cp, P, H, t, w, M, mt, mw, CC, 0);
	prob += cprob;
      }
    if( fabs(1-prob) > epsilonDistr || 1 )
      printf("P_cc1(.|%s %s %s %s %s %s %s CC): %.10f\n", labelIndex[P], labelIndex[H], labelIndex[t], wordIndex[w], labelIndex[M], labelIndex[mt], wordIndex[mw], prob);

    prob = 0;
    for(cp=1; cp<=nlabelIndex; cp++)
      if( !isnt[cp] && properLabel(cp) ){
	cprob = computeCP1Prob(cp, P, H, t, w, M, mt, mw, PUNC, 0);
	prob += cprob;
      }
    if( fabs(1-prob) > epsilonDistr || 1 )
      printf("P_punc1(.|%s %s %s %s %s %s %s PUNC): %.10f\n", labelIndex[P], labelIndex[H], labelIndex[t], wordIndex[w], labelIndex[M], labelIndex[mt], wordIndex[mw], prob);
  
    sample++;
  }
  printf("CP1 prob distributions verified\n");

  /* cp2 */
  sample = 0;
  while( sample < cp2Sample ){
    P = getRandLabel(1);
    H = getRandLabel(0);
    t = getRandLabel(2);
    w = getRand1ToN(NLEX);
    M = getRandLabel(0);
    mt = getRandLabel(2);
    mw = getRand1ToN(NLEX);
    cp = getRandLabel(2);

    prob = computeCP2Prob(UNKINDEX, cp, P, H, t, w, M, mt, mw, CC, 0);
    for(cpw=1, cnt=0; cpw<=nwordIndex; cpw++){
      cprob = computeCP2Prob(cpw, cp, P, H, t, w, M, mt, mw, CC, 0);
      prob += cprob;
      cnt++;
    }
    if( fabs(1-prob) > epsilonDistr || 1 )
      printf("P_cc2(.|%s %s %s %s %s %s %s %s CC): %.10f\n", labelIndex[cp], labelIndex[P], labelIndex[H], labelIndex[t], wordIndex[w], labelIndex[M], labelIndex[mt], wordIndex[mw], prob);


    prob = computeCP2Prob(UNKINDEX, cp, P, H, t, w, M, mt, mw, PUNC, 0);
    for(cpw=1; cpw<=nwordIndex; cpw++){
      cprob = computeCP2Prob(cpw, cp, P, H, t, w, M, mt, mw, PUNC, 0);
      prob += cprob;
    }
    if( fabs(1-prob) > epsilonDistr || 1 )
      printf("P_punc2(.|%s %s %s %s %s %s %s %s PUNC): %.10f\n", labelIndex[cp], labelIndex[P], labelIndex[H], labelIndex[t], wordIndex[w], labelIndex[M], labelIndex[mt], wordIndex[mw], prob);

    sample++;
  }
  printf("CP2 prob distributions verified\n");
  
}
  
int getRand1ToN(int N)
{
  return rand()%N+1;
}

int getRandLabel(int flag)
{
  int i;

  while( 1 ){
    i = rand()%NNTS+1;
    if( !properLabel(i) ) continue;
    if( flag==0 ) return i;
    if( flag==1 )
      if( isnt[i] ) return i;
    if( flag==2 )
      if( !isnt[i] ) return i;
  }
}

int properLabel(int L)
{
  char *s;

  return 1;
  s = labelIndex[L]; 

  /* ignored labels */
  if( !strcmp(s, "``") || !strcmp(s, "''") || !strcmp(s, ".") )
    return 0;

  /* special labels */
  if( !strcmp(s, "S1") )
    return 0;

  return 1;

}
