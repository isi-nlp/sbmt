#include "tree.h"
#include "myalloc.h"
#include "hash.h"

int GOLDTREE = 0;
int allb;
Hash hashCnt;

int main(int argc, char **argv)
{
  FILE *f;
  Tree *tree;
  int c, i, k, nt, w;
  int b, e, flag, TREEFLAG, bug;
  char buf[200];

  if( argc<3 ){
    fprintf(stderr, "Usage: %s TREEFLAG file-list\n", argv[0]);
    exit(1);
  }

  mymalloc_init();
  mymalloc_char_init();
  createHash(4000003, &hashCnt);

  readGrammar("../GRAMMAR/PTB");

  TREEFLAG = atoi(argv[1]);
  for(k=2; k<argc; k++){
    if( (f=fopen(argv[k], "r"))==NULL)
      { fprintf(stderr, "Cannot open file %s\n", argv[k]); exit(2); }
    
    nt = 0;
    c = fgetc(f);
    while( c=='\n' || c=='\t' || c==' ')
      c = fgetc(f);
    while( c=='(' || c=='0' )
      {
	if( TREEFLAG==1 ){
	  tree = readFullTree(f, 0, &bug);
	  updatePruning(tree); /* remove '' `` and . */
	}
	else
	  tree = readTree(f, 2);
      
	if( tree ){
	  postprocessTree(tree);
          writeTreeSingleLine(tree);
	}
	else{
	  printf("(TOP RaduErrorParse/NN)\n");
	}

	c = fgetc(f);
	while( c=='\n' || c=='\t' || c==' ')
	  c = fgetc(f);
	if( c=='=' ){ /* for Nbest lists */
	  printf("%c", c);
	  while( c=='=' ){
	    c = fgetc(f);
	    printf("%c", c);
	  }
	  while( c=='\n' || c=='\t' || c==' ')
	    c = fgetc(f);
	}
      }
    fclose(f);
  }
}
