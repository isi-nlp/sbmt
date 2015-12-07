#include "tree.h"
#include "model2.h"
#include "model2tree.h"

#define MAXSENT 2000
#define MAXNBEST 100

int CONVERT2MT;
int SHOWTREE = 1;

Tree *tree[MAXSENT][MAXNBEST];
int ntree[MAXSENT];
int nt;

int strChunks, mallocChunksKey, mallocChunksNode, mallocChunksTable, mallocChunksHash;
int allb;

void assignProb();
int treeConstit(Tree *tree, char constit[][LLEN], int n);

int main(int argc, char **argv)
{
  FILE *f;
  char buf[SLEN], Training[SLEN], Grammar[SLEN];
  int i, j, bug, c, mode, pos;
  double p;

  if( argc!=5 )
    { fprintf(stderr, "Usage: %s treefile mode Train:PTB|MT pos\n", argv[0]); exit(-1); }
  mode = atoi(argv[2]); /* mode==0 pre-preprocesses the tree(s); mode==2 does not; mode==1 takes full tree */

  if( !strcmp(argv[1], "stdin") )
    f = stdin;
  else
    if( (f=fopen(argv[1], "r"))==NULL)
      { fprintf(stderr, "Cannot open file %s\n", argv[1]); exit(2); }

  sprintf(Training, "../TRAINING/rules.%s", argv[3]);
  sprintf(Grammar, "../GRAMMAR/%s", argv[3]);

  if( strcmp(argv[3], "PTB") ){
    CONVERT2MT = 1;
    FFACTOR = FFACTOR_M2 = 20;
  }
  THCNT = 11;

  pos = atoi(argv[4]);

  SBLM = 1;
  if( SBLM )
    fprintf(stderr, "Using SBLM=1 means duplicate training: under THCNT, duplicate lex as UNK\n");
  else
    fprintf(stderr, "Using SBLM=0 means regular training, and Collins style treatment of UNK\n"); 
  loadSBLM(Training, Grammar, THCNT);

  nt = 0; i = 0;
  c = fgetc(f);
  while( (c=='\n' || c=='\t' || c==' ') )
    c = fgetc(f);
  while( c=='(' || c=='0' )
    {
      if( c!='0' ){
	if( mode==1 ){
	  tree[nt][i] = readFullTree(f, CONVERT2MT, &bug);
	  updatePruning(tree[nt][i]); /* remove `` '' and . */
	  if( CONVERT2MT )
	    convertPTBTree2MTTree(tree[nt][i]);
	  i++;
	}
	else
	  tree[nt][i++] = readTree(f, mode);
      }
      
      c = fgetc(f);
      while( (c=='\n' || c=='\t' || c==' ') )
	c = fgetc(f);
      
      if( c=='=' ){ /* for Nbest lists marker */
	while( c!='\n' ){
	  c = fgetc(f);
	}
	while( c=='\n' || c=='\t' || c==' ')
	  c = fgetc(f);
	ntree[nt] = i;
	nt++; i=0;
	if( nt >= MAXSENT ){
	  fprintf(stderr, "Warning: limit of MAXSENT reached; no more tree reading\n"); break;
	}
      }
    }
  
  fprintf(stderr, "Trees to be scored: %d\n", nt);
  for(i=0; i<nt; i++){
    for(j=0; j<ntree[i]; j++){
      if( j+1==pos ){
	p = model2Prob(tree[i][j]);
	printf("%.5f %d\n", p, i+1);
      }
    }
  }
}
