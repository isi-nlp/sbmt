#include "grammar.h"

char *labelIndex[MAXNTS], *wordIndex[MAXVOC];
int nlabelIndex, nwordIndex;
int unaryC[MAXNTS][MAXNTS], nunary[MAXNTS];
char *unaryL[MAXNTS][MAXNTS];
int lframe[MAXNTS][MAXNTS][MAXSUBCATS], lframeC[MAXNTS][MAXNTS][MAXSUBCATS], nlframe[MAXNTS][MAXNTS];
int rframe[MAXNTS][MAXNTS][MAXSUBCATS], rframeC[MAXNTS][MAXNTS][MAXSUBCATS], nrframe[MAXNTS][MAXNTS];

char *wordTag[MAXVOC][MAXTAGS];
int nwordTag, wordTagC[MAXVOC][MAXTAGS], cnwordTag[MAXVOC];
int wordTagI[MAXVOC][MAXTAGS], totalWordCnt;

void extractNodeInfo(int L, int cnt)
{
  Key k;
  unsigned char fkey[NKEY];

  fkey[0] = L; fkey[1] = 91; 
  k.key = fkey; k.len = 2;
  addElementJmpHash(&k, &jmphashCnt, cnt);

}

void extractLeafInfo(char *preT, int t, int w, int cnt, int recordFlag)
{
  Key k;
  unsigned char fkey[NKEY];
  int c, l;

  l=0; l=writeWord(fkey, w, l); fkey[l++] = 93;
  k.key = fkey; k.len = l;
  addElementJmpHash(&k, &jmphashCnt, cnt); 

  l=0; l=writeWord(fkey, w, l); fkey[l++] = t; fkey[l++] = 95;
  k.key = fkey; k.len = l;
  c = addElementJmpHash(&k, &jmphashCnt, cnt);
  if( c==cnt && recordFlag ){ /* new word/tag pair */
    insertSortLex(preT, wordTag[w], wordTagC[w], &cnwordTag[w]);
    if( cnwordTag[w] >= MAXTAGS )
      { fprintf(stderr, "Error: MAXTAGS limit reached\n"); exit(16); }
  }
}

int getLabelIndex(char *label, int addflag)
{
  int l, idx;
  Key k;
  unsigned char fkey[100];

  if( !strcmp(label, "+STOP+") )
    return 0;

//  if( !strcmp(label, "+r+") ) printf("");

  l = strlen((char*)label);
  sprintf((char*)fkey, "%s", label);
  fkey[l++] = 101; 
  k.key = fkey; k.len = l;
  if( (idx=findElementHash(&k, &hashCnt)) )
    return idx;

  if( !addflag ){
    if( strcmp(label, ".") && strcmp(label, "''") && strcmp(label, "``") && strcmp(label, "TOP") )
      return -1; 
    else{
      return 0;
      /* fprintf(stderr, "Label %s is unknown\n", label);  */
    }
  }
  
  idx = ++nlabelIndex;
  if( idx>=MAXNTS )
    { fprintf(stderr, "Error: number MAXNTS reached with label %s\n", label); exit(23); }

  labelIndex[idx] = strdup(label);
  addElementHash(&k, &hashCnt, idx); 
  return idx;
}

int getWordIndex(char *word, int addflag)
{
  int l, idx;
  Key k;
  char fkey[100];

  if( !strcmp(word, "+STOP+") )
    return 0;
  
  l = strlen(word);
  sprintf(fkey, "%s", word);
  fkey[l++] = 102; 
  k.key = (unsigned char*)fkey; k.len = l;
  if( (idx=findElementHash(&k, &hashCnt)) )
    return idx;

  if( !addflag ){
    if( strcmp(word, ".") && strcmp(word, "''") && strcmp(word, "``") && 0 )
      fprintf(stderr, "Word %s is unknown\n", word); 
    return UNKINDEX; 
  }
  else if( addflag==2 ){
    fprintf(stderr, "Word %s is unknown by the training events used\n", word); exit(33);
  }

  idx = ++nwordIndex;
  if( idx==UNKINDEX ) idx = ++nwordIndex;
  if( idx==NAV ) idx = ++nwordIndex;
  if( nwordIndex >= MAXVOC )
    { fprintf(stderr, "Error: maximum number of words in vocabulary exceeded: %d\n", nwordIndex); exit(24); } 
  addElementHash(&k, &hashCnt, idx); 
  wordIndex[idx] = strdup(word);
  return idx;
}


void updateFrameList(char *parent, int p, char *head, int h, int frameL, int frameR)
{
  int i;

  for(i=0; i<nunary[h]; i++)
    if( !strcmp(unaryL[h][i],parent) ){
      unaryC[h][i]++;
      break;
    }
  if( i==nunary[h] )
    insertSortLex(parent, unaryL[h], unaryC[h], &nunary[h]);

  for(i=0; i<nlframe[p][h]; i++)
    if( lframe[p][h][i]==frameL ){
      lframeC[p][h][i]++;
      break;
    }
  if( i==nlframe[p][h] )
    insertSortInt(frameL, lframe[p][h], lframeC[p][h], &nlframe[p][h]);

  for(i=0; i<nrframe[p][h]; i++)
    if( rframe[p][h][i]==frameR ){
      rframeC[p][h][i]++;
      break;
    }
  if( i==nrframe[p][h] )
    insertSortInt(frameR, rframe[p][h], rframeC[p][h], &nrframe[p][h]);

}

int encodeFrame(char *frame)
{
  int pos, ef = 0;
  char subcat[10], *cframe;

  if( !strcmp(frame, "{}") ) return 0;

  cframe = frame+1; /* past "{" */
  while(1){
    sscanf(cframe, "%s", subcat);
    if( !strcmp(subcat, "}") ) return ef;
    pos = mapSubcat(subcat);
    ef += (int)pow(10, pos);
    cframe += strlen(subcat)+1;
  }
}

int mapSubcat(char *label)
{
  if( !strcmp(label, "NP-C") ) 
    return 4;
  if( !strcmp(label, "S-C") || !strcmp(label, "SG-C") ) 
    return 3;
  if( !strcmp(label, "SBAR-C") ) 
    return 2;
  if( !strcmp(label, "VP-C") ) 
    return 1;
  return 0;
}

int writeWord(unsigned char* string, int n, int i)
{
  if( WORD_ON_2_BYTES )
    return write2Bytes(string, n, i);
  else
    return write3Bytes(string, n, i);
}


int writeFrame(unsigned char* string, int n, int i)
{
  return write2Bytes(string, n, i);
}

int write2Bytes(unsigned char* string, int n, int i)
{
  string[i++] = (n & 255);
  string[i++] = n/256;
  return i;
}

int write3Bytes(unsigned char* string, int n, int i)
{
  int m;

  string[i++] = (n & 255);
  m = n/256;
  string[i++] = (m & 255);
  string[i++] = m/256;
  return i;
}

void insertSortLex(char *s, char* list[], int listC[], int *nlist)
{
  int i, j;

  for(i=0; i<*nlist; i++){ /* insert sort */
    if( lessLex(s, list[i]) )
      break;
  }
  for(j=*nlist; j>=i+1; j--){
    list[j] = list[j-1];
    listC[j] = listC[j-1];
  }
  list[i] = strdup(s);
  listC[j] = 1;
  (*nlist)++;
}

void insertSortInt(int s, int list[], int listC[], int *nlist)
{
  int i, j;

  for(i=0; i<*nlist; i++) /* insert sort */
    if( s<list[i] )
      break;
  for(j=*nlist; j>=i+1; j--){
    list[j] = list[j-1];
    listC[j] = listC[j-1];
  }
  list[i] = s;
  listC[i] = 1;
  (*nlist)++;
}

int lessLex(char *r1, char *r2)
{
  int i;

  if( !strcmp(r1, r2) ) return 0;
  for(i=0; i<strlen(r1) && i<strlen(r2); i++){
    if( r1[i]<r2[i] ) return 1;
    if( r1[i]>r2[i] ) return 0;
  }
  if( i==strlen(r1) ) return 1;
  if( i==strlen(r2) ) return 0;
  fprintf(stderr, "Error: comparing %s and %s\n", r1, r2);
  return 0;
}
