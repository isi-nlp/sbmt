#include "model2.h"

int strChunks, mallocChunksKey, mallocChunksNode, mallocChunksTable, mallocChunksHash;
int allb;

void createArray(char *buf, int arr[], char *abuf);

int main(int argc, char **argv)
{
  FILE *f;
  char buf[SLEN], events[SLEN], grammar[SLEN], abuf[SLEN], vocSuffix[SLEN];
  int c, nt, i, arr[100], L, t, w, cwVOC;
  double p;

  strcpy(buf, "S NP-C VP .");
  i = ruleHead(buf);
  printf("%d : %s\n", i, buf);  

  strcpy(buf, "NP-C NPB PP SBAR");
  i = ruleHead(buf);
  printf("%d : %s\n", i, buf);  

  strcpy(buf, "NP NPB CC NPB");
  i = ruleHead(buf);
  printf("%d : %s\n", i, buf);  

  strcpy(buf, "VP S CC VP");
  i = ruleHead(buf);
  printf("%d : %s\n", i, buf);  

  if( argc!=3 )
    { fprintf(stderr, "Usage: %s wVOC PTB|MT|PTB-MT\n", argv[0]); exit(-1); }

  cwVOC = atoi(argv[1]);
  sprintf(events, "../TMP/events.%s", argv[2]); 
  sprintf(grammar, "../GRAMMAR/%s", argv[2]); 
  strcpy(vocSuffix, "vocab.little0-16.all+lhs-norm");

  if( cwVOC ){
    initAPI(1, 1, 1, 1);
    initSBLM(events, grammar, 0, 6);
    trainSBLMpps(events, vocSuffix, cwVOC);	
  }
  else{
    initAPI(1, 1, 0, 0);
    initSBLM(events, grammar, 0, 6);
  }

  strcpy(buf, "VP~VBZ~is");
  p = probSBLMprior(buf);
  printf("%.5f : %s\n", p, buf);  

  strcpy(buf, "NPB~NNS~people");
  p = probSBLMprior(buf);
  printf("%.5f : %s\n", p, buf);  

  strcpy(buf, "NPB~NNS~people -> NNS~NNS~people : CD~CD~7~0 DT~DT~These~0 : :");
  p = probSBLM(buf, 0); printf("%.5f : %s\n", p, buf);
  p = probSBLM(buf, 1); printf("%.5f : %s\n", p, buf);
  p = probSBLM(buf, 2); printf("%.5f : %s\n", p, buf);
  p = probSBLM(buf, 3); printf("%.5f : %s\n", p, buf);

  createArray(buf, arr, abuf);
  p = probSBLM2(arr, 0); printf("%.5f : %s\n", p, abuf);
  p = probSBLM2(arr, 1); printf("%.5f : %s\n", p, abuf);
  p = probSBLM2(arr, 2); printf("%.5f : %s\n", p, abuf);
  p = probSBLM2(arr, 3); printf("%.5f : %s\n", p, abuf);

  strcpy(buf, "PP~IN~from -> IN~IN~from : : NP-C~NNP~France~0 :");
  p = probSBLM(buf, 3);
  printf("%.5f : %s\n", p, buf);

  strcpy(buf, "NP-C~NNP~France -> NP~NNP~France : : CC~CC~and~0 NP~NNP~Russia~0 :");
  p = probSBLM(buf, 3);
  printf("%.5f : %s\n", p, buf);

  strcpy(buf, "VP~VBP~include -> VBP~VBP~include : : NP-C~NNS~astronauts~1 :");
  p = probSBLM(buf, 0); printf("%.5f : stopFlag = %d : %s\n", p, 0, buf);
  p = probSBLM(buf, 1); printf("%.5f : stopFlag = %d : %s\n", p, 1, buf);
  p = probSBLM(buf, 2); printf("%.5f : stopFlag = %d : %s\n", p, 2, buf);
  p = probSBLM(buf, 3); printf("%.5f : stopFlag = %d : %s\n", p, 3, buf);

  createArray(buf, arr, abuf);
  p = probSBLM2(arr, 0); printf("%.5f : stopFlag = %d : %s\n", p, 0, buf);
  p = probSBLM2(arr, 1); printf("%.5f : stopFlag = %d : %s\n", p, 1, buf);
  p = probSBLM2(arr, 2); printf("%.5f : stopFlag = %d : %s\n", p, 2, buf);
  p = probSBLM2(arr, 3); printf("%.5f : stopFlag = %d : %s\n", p, 3, buf);

  printf("unknownSBLM(%d)=%d\n", getWordIndex("fifa", 0),  unknownSBLM(getWordIndex("fifa", 0)));
  printf("unknownSBLM(%d)=%d\n", getWordIndex("gunmen", 0),  unknownSBLM(getWordIndex("gunmen", 0)));
  printf("unknownSBLM(%d)=%d\n", getWordIndex("men", 0),  unknownSBLM(getWordIndex("men", 0)));

  strcpy(buf, "S~VBD~declared -> VP~VBD~declared : ADVP~RB~also~0 NP-C~NNP~fifa~0 : :");
  createArray(buf, arr, abuf);
  p = probSBLM2(arr, 3); printf("%.5f : %s\n", p, abuf);

  L = getLabelIndex("S", 0); t = getLabelIndex("VBD", 0); w = getWordIndex("was", 0);
  p = hashedComputePriorProb(L, t, w);
  printf("%.5f\n", p);
  p = hashedComputePriorProb(L, t, w);
  printf("%.5f\n", p);

  L = getLabelIndex("NP", 0); t = getLabelIndex("NNS", 0); w = getWordIndex("rights", 0);
  p = hashedComputePriorProb(L, t, w);
  printf("%.5f\n", p);

  L = getLabelIndex("NP", 0); t = getLabelIndex("DT", 0); w = getWordIndex("those", 0);
  p = hashedComputePriorProb(L, t, w);
  printf("%.5f\n", p);

} 

void createArray(char *node, int arr[], char *anode)
{
  char buf[SLEN];
  int i, j, cv, P, t, w, M[MAXMODS], mt[MAXMODS], mw[MAXMODS], nL=0, nR=0;

  sscanf(node, "%s", buf); /* parent info */
  readNode(buf, "parent", &P, &t, &w, 0);
  arr[0] = P; arr[1] = t; arr[2] = w;
  node += strlen(buf)+1;

  sscanf(node, "%s", buf); /* -> */
  node += strlen(buf)+1;

  sscanf(node, "%s", buf); /* head info */
  readNode(buf, "head", &(M[0]), &(mt[0]), &(mw[0]), 0);
  arr[3] = M[0]; arr[4] = mt[0]; arr[5] = mw[0];
  node += strlen(buf)+1;

  sscanf(node, "%s", buf); /* : */
  node += strlen(buf)+1;

  sscanf(node, "%s", buf); 
  node += strlen(buf)+1;
  nL = 0; i = 6;
  while( 1 ){
    if( !readNode(buf, "modL", &(M[nL]), &(mt[nL]), &(mw[nL]), &cv) ) break;
    arr[i++] = M[nL]; arr[i++] = mt[nL]; arr[i++] = mw[nL]; arr[i++] = cv;
    nL++;
    sscanf(node, "%s", buf); 
    node += strlen(buf)+1;
  }
  arr[i++] = -1;

  sscanf(node, "%s", buf); 
  node += strlen(buf)+1;
  nR = 0;
  while( 1 ){
    if( !readNode(buf, "modR", &(M[nR]), &(mt[nR]), &(mw[nR]), &cv) ) break;
    arr[i++] = M[nR]; arr[i++] = mt[nR]; arr[i++] = mw[nR]; arr[i++] = cv;
    nR++;
    sscanf(node, "%s", buf); 
    node += strlen(buf)+1;
  }
  arr[i++] = -2;
  
  anode[0] = 0;
  for(i=0; arr[i]!=-2; i++){
    sprintf(buf, "%d ", arr[i]);
    strcat(anode, buf);
  }
  strcat(anode, "-2");
}
