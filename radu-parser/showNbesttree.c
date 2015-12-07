#include "tree.h"
#include "myalloc.h"

#define MAXSENT 2000
#define MAXNBEST 100

int FULLTREE = 0;
int allb;

int main(int argc, char **argv)
{
  FILE *f;
  Tree *tree[MAXSENT][MAXNBEST];
  int c, i, k, j, nt, w, ntree[MAXSENT];
  int b, e, flag;
  char buf[200];

  if( argc!=5 ){
    fprintf(stderr, "Usage: showNbesttree sent-number nbest-number tree-list fulltreeFlag\n"); exit(11);
  }

  k = atoi(argv[1])-1;
  j = atoi(argv[2])-1;

  if( (f=fopen(argv[3], "r"))==NULL)
    { fprintf(stderr, "Cannot open file %s\n", argv[3]); exit(2); }
  
  FULLTREE = atoi(argv[4]);

  c = fgetc(f);
  while( c=='\n' || c=='\t' || c==' ')
    c = fgetc(f);
  nt = 0; i = 0;
  while( c=='(' || c=='0' )
    {
      if( FULLTREE )
	tree[nt][i++] = readFullTree(f);
      else
	tree[nt][i++] = readTree(f, 2);
      
      c = fgetc(f);
      while( c=='\n' || c=='\t' || c==' ')
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
    
      if( nt>k ){
	if( ntree[k]<j )
	  fprintf(stderr, "There were only %d trees for sentence %d given \n", ntree[k], k);
	else
	  writeTree(tree[k][j], 0, 0, FULLTREE);
	break;
      }
    }
}

