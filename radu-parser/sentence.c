#include "sentence.h"

void readSentences(char *fname)
{
  FILE *f;
  int i, n, rn, tagI, wordI, pflag, stopFlag;
  char word[MAXCHARS], tag[MAXCHARS], line[20000], sn[10], *p;

  labelIndex_Colon = getLabelIndex(":", 0);
  labelIndex_Comma = getLabelIndex(",", 0);
  labelIndex_LRB = getLabelIndex("-LRB-", 0);
  labelIndex_RRB = getLabelIndex("-RRB-", 0);  

  if( !strcmp(fname, "stdin") )
    f = stdin;
  else
    if( (f=fopen(fname, "r"))==NULL)
      { fprintf(stderr, "Cannot open file %s\n", fname); exit(2); }

  nsent = 0;
  while( fgets(line, 20000, f)!=NULL ){
    fprintf(stderr, "Reading sentence %d\n", nsent+1);
    p = line;
    sscanf(p, "%s", &sn); p += strlen(sn)+1;
    n = atoi(sn);
    if( n<=0 ){ break; }
    for(i=0, rn=0, stopFlag=0; i<n; i++){
      sscanf(p, "%s %s", word, tag); p+= strlen(word) + strlen(tag) + 2;
      if( stopFlag ) continue;
      /* PTB-like input should have the conversion already done; MT-like input does not need the conversion
      if( !strcmp(word, "(") ){ strcpy(word, "-LRB-"); }
      if( !strcmp(word, ")") ){ strcpy(word, "-RRB-"); }
      */
      strcpy(origsent[nsent].word[i], word); strcpy(origsent[nsent].tag[i], tag); 
      tagI = getLabelIndex(tag, 0);
      wordI = getWordIndex(word, 0);
      if( i>0 && (tagI==labelIndex_Colon || tagI==labelIndex_Comma) ){ 
	sent[nsent].hasPunc[rn-1] = 1; 
	strcpy(sent[nsent].wordPunc[rn-1], word); strcpy(sent[nsent].tagPunc[rn-1], tag);
	sent[nsent].wordIPunc[rn-1] = wordI; sent[nsent].tagIPunc[rn-1] = tagI;
	continue; 
      }
      if( strcmp(tag, ".") && strcmp(tag, "''") && strcmp(tag, "``") ){
	strcpy(sent[nsent].word[rn], word); strcpy(sent[nsent].tag[rn], tag);
	sent[nsent].wordI[rn] = wordI; sent[nsent].tagI[rn] = tagI;	
	sent[nsent].hasPunc[rn] = 0;
	sent[nsent].wordPunc[rn][0] = 0; sent[nsent].wordIPunc[rn] = 0; 
	sent[nsent].tagPunc[rn][0] = 0; sent[nsent].tagIPunc[rn] = 0; 
	rn++;
	if( rn>=MAXWORDS || n>=MAXWORDS ){
	  rn = -1; stopFlag = 1;
	  /* fprintf(stderr, "Error: MAXWORDS limit reached\n"); exit(17);  */
	}
      }
    }
  
    for(i=0, pflag=0; i<rn; i++){
      if( sent[nsent].tagI[i]==labelIndex_LRB ) pflag = 1;
      if( sent[nsent].tagI[i]==labelIndex_RRB ) pflag = 0;
      if( pflag==0 )
	sent[nsent].hasPunc4Prune[i] = sent[nsent].hasPunc[i];
      else
	sent[nsent].hasPunc4Prune[i] = 0;
    }
 
    sent[nsent].nwords = rn;
    origsent[nsent].nwords = n;
    nsent++;
    if( nsent >= MAXSENT )
      { fprintf(stderr, "Error: MAXSENT limit reached\n"); exit(16); }
  }
  close(f);
}

void writeFlatTree(FILE *f)
{
  int i;

  fprintf(f, "(TOP ");
  for(i=0; i<corigsent->nwords; i++)
    fprintf(f, "(%s %s) ", corigsent->tag[i], corigsent->word[i]);
  fprintf(f, ")\n");
}

