#include "tree.h"

FILE *f;
int allb;

int main(int argc, char **argv)
{
  Tree *tree;
  int ntree=0, flag, bcnt;
  int c, pc, i, j, k, n, nt, ndt, bug;
  char fname[200];

  for(i=1; i<argc; i++){
    if( (f=fopen(argv[i], "r"))==NULL)
      { fprintf(stderr, "Cannot open file %s\n", argv[i]); exit(2); }
    strcpy(fname, argv[i]);
 
    nt = 1; bcnt = 0;
    c = fgetc(f);
    while( c=='\n' || c=='\t' || c==' ')
      c = fgetc(f);
    while( c=='(' )
      {
	tree = readFullTree(f, 0, &bug);
	/* tree = readTree(f, -1);  */
	writeLeaves(tree, 1);
	printf("\n");
	c = fgetc(f); pc = c;
	while( c=='\n' || c=='\t' || c==' '){
	  c = fgetc(f);
	  if( pc=='\n' && c=='\n' ) printf("\n");
	  pc = c;
	}
	freeTree(tree);
      }
    fclose(f);
  }
}
