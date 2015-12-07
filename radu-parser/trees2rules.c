#include "hash.h"
#include "tree.h"
#include "rules.h"

#define MAXC 5000

#define CONVERT2MT 1

Hash hashCnt;

int main(int argc, char **argv)
{
  Tree *tree;
  int ntree=0, flag, start, end, bcnt, nlines;
  FILE *f, *B;
  int c, i, j, k, n, nt, ndt, bug;
  char buf[MAXC], bugf[200];

  /* assumes 1-tree-per-line format in order to use arbitrary start-pints */
  if( argc != 4 )
    { fprintf(stderr, "Usage: %s treefile start-point end-point\n", argv[0]); exit(22); }

  mymalloc_init();
  mymalloc_char_init();
  createHash(4000003, &hashCnt);
  readGrammar("../GRAMMAR/PTB2-MT"); /* the "bug" guards must know about CJJ, etc */

  strcpy(MISC_SUBCAT, "MISC-C");
  strcpy(S_SUBCAT, "S-C");
  strcpy(START_TAU, "+START+");

  if( (f=fopen(argv[1], "r"))==NULL)
    { fprintf(stderr, "Cannot open file %s\n", argv[1]); exit(2); }

  start = atoi(argv[2]);
  end = atoi(argv[3]);

  sprintf(bugf, "BUGS/bugIndex.%d-%d", start, end);
  if( (B=fopen(bugf, "w"))==NULL)
    { fprintf(stderr, "Cannot open file %s\n", bugf); exit(2); }

  for(i=0; i<start; i++)
    if( fgets(buf, MAXC, f)==NULL )
      { fprintf(stderr, "There are only %d trees in %s\n", i, argv[1]); exit(23); }

  nlines = 1;
  nt = 1; bcnt = 0; 
  c = fgetc(f);
  while( c=='\n' || c=='\t' || c==' ')
    c = fgetc(f);
  while( c=='(' )
    {
      tree = readFullTree(f, CONVERT2MT, &bug);
      if( bug ){
	fprintf(B, "%d\n", nlines);
	bcnt++;
      }
      /* else /* now it includes the bug trees: the "Turkey" bug is fixed, and bug trees are 
	 only caused by the pos-tagger, which assigns , POS to some unknown words (such as "Li", "[",  "]", etc.) */ 
      {
	updatePruning(tree); /* remove `` '' and . */
	if( CONVERT2MT )
	  convertPTBTree2MTTree(tree);
	extractPlainRules(tree, nt);
      }
      nt++;
      if( nt>=end-start )
	break;

      c = fgetc(f);
      if( c=='\n' ) nlines++;
      while( c=='\n' || c=='\t' || c==' '){
	c = fgetc(f);
	if( c=='0' ){  /* failed parse tree */
	  nt++; 
	  c = fgetc(f);
	}
	if( c=='\n' ) nlines++;
      }
      if( nlines != nt )
	printf(""); /* blank lines make the mismatch */
    }
  fclose(f);
  fclose(B);
  fprintf(stderr, "Successfully transformed %d trees into rules (Bug trees: %d)\n", nt, bcnt);
  fprintf(stderr, "Starting point: %d Ending point: %d Next starting point: %d\n", start, start+nt-1, start+nt);
  return 0;
}

