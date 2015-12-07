#include "tree.h"
#include "myalloc.h"
#include "hash.h"

int TREEFLAG = 1; /* 0 means expanding the tree; 1 means full tree; 2 means not-expand tree */
int allb;
Hash hashCnt;

int main(int argc, char **argv)
{
  FILE *f;
  Tree *tree[100000];
  int c, i, k, nt, w, bug;
  int b, e, flag;
  char buf[200], rst[100];

  if( argc!=4 ){
    fprintf(stderr, "Usage: showtree tree-number tree-list treeFlag\n"); exit(11);
  }

  k = atoi(argv[1]);

  if( (f=fopen(argv[2], "r"))==NULL)
    { fprintf(stderr, "Cannot open file %s\n", argv[2]); exit(2); }
  
  TREEFLAG = atoi(argv[3]);

  mymalloc_init();
  mymalloc_char_init();
  createHash(4000003, &hashCnt);

  readGrammar("../GRAMMAR/PTB");

  nt = 1;
  c = fgetc(f);
  while( c=='\n' || c=='\t' || c==' ')
    c = fgetc(f);
  while( c=='(' || c=='0' )
    {
      if( TREEFLAG==1 )
	tree[nt] = readFullTree(f, 0, &bug);
      else
	tree[nt] = readTree(f, TREEFLAG);
      
      if(nt==k){
	writeTree(tree[k], 0, 0);
	break;
      }

      nt++;
      c = fgetc(f);
      while( c=='\n' || c=='\t' || c==' ')
	c = fgetc(f);
    }
  fclose(f);
    
  if( nt<k )
    fprintf(stderr, "There are only %d in this list\n", nt);
}
