#include "tree.h"
#include "model2.h"
#include "model2tree.h"

int VAR;
int CONVERT2MT;
int SHOWTREE;
int SHOWPROB;

Tree *tree;
int nt;

int strChunks, mallocChunksKey, mallocChunksNode, mallocChunksTable, mallocChunksHash;
int allb;

int treeConstit(Tree *tree, char constit[][LLEN], int n);
void writeAPIprobs(Tree *tree);

int main(int argc, char **argv)
{
  FILE *f;
  char buf[SLEN], Training[SLEN], Grammar[SLEN], *trainDir, *eventDir, *RTYPE;
  int i, bug, c, cc, mode, flag;
  char constit[MAXC][LLEN], wbuf[20000];
  int j, k, nconstit, wcnt, cnt, flagPPS;
  double e, p, wp;

  if( argc!=6 && argc!=8 )
    { fprintf(stderr, "Usage: %s treefile mode Train:PTB|PTB2-MT THCNT SBLM [train-dir cnt]\n", argv[0]); exit(-1); }
  mode = atoi(argv[2]); /* mode==0 pre-preprocesses the tree(s); mode==2 does not; mode==1 takes full tree */

  if( !strcmp(argv[1], "stdin") )
    f = stdin;
  else
    if( (f=fopen(argv[1], "r"))==NULL)
      { fprintf(stderr, "Cannot open file %s\n", argv[1]); exit(2); }

  initAPI(1, 1, 1, 1); /* some of its settings may be changed in what follows */
  
  RTYPE = argv[3];
  THCNT = atoi(argv[4]);
  SBLM = atoi(argv[5]);
  if( argc==6 ){
    wVOC = 0;
    sprintf(Grammar, "../GRAMMAR/%s/", RTYPE);
    sprintf(Training, "../TRAINING/rules.%s", RTYPE);
  }
  else{
    wVOC = 1;
    cnt = atoi(argv[7]);
    eventDir = argv[6];
    trainDir = strdup(argv[6]);
    for(i=strlen(trainDir)-2, flag=0; i>=0; i--){ 
      if( trainDir[i]=='/' ){ flag++; }
      if( flag==2 ) break;
    }
    trainDir[i+1] = 0; strcat(trainDir, "sblm");
    sprintf(Grammar, "%s/GRAMMAR/%s/", trainDir, RTYPE);
    sprintf(Training, "%s/TRAINING/rules.%s", trainDir, RTYPE);
  }
  

  if( SBLM )
    fprintf(stderr, "Using SBLM=1 means SBLM training, with uniform treatment of UNK\n");
  else{
    fprintf(stderr, "Using SBLM=0 means regular training, and Collins style treatment of UNK\n"); 
    if( wVOC==1 )
      { fprintf(stderr, "Cannot use voc-pruned training with SBLM=0\n"); exit(1);}
  }

  if( wVOC==1 )
    PPS = 1;
  else
    PPS = 0;
  initSBLM(Training, Grammar, THCNT);

  flagPPS = PPS;
  while( 1 ){
    nt = 0; i = 0;
    c = fgetc(f);
    while( (c=='\n' || c=='\t' || c==' ') )
      c = fgetc(f);
    while( c=='(' || c=='0' || c =='=' )
    {
      if( flagPPS ){
	trainSBLMpps(eventDir, RTYPE, cnt);	
	flagPPS = 0;
      }

      if( c!='0' && c!='=' ){
	/* fprintf(stderr, "Scoring tree (%d,%d)\n", nt, i); */
	if( mode==1 ){
	  tree = readFullTree(f, CONVERT2MT, &bug);
	  updatePruning(tree); /* remove `` '' and . */
	  if( CONVERT2MT )
	    convertPTBTree2MTTree(tree);
	}
	else
	  tree = readTree(f, mode);
	i++;

	wp = 0; wbuf[0] = 0; wcnt = 0;
	p = model2Prob(tree, &wcnt, &wp, wbuf);
	
	SHOWTREE = 0;
	SHOWPROB = 1;    
	if( SHOWTREE ){
	  printf("%d ", i);
	  writeTreeSingleLine(tree, 1);
	  printf("\n");
	  /* writeAPIprobs(tree); */
	}
	if( SHOWPROB )
	  printf("%.5f\n", p); 
	/* printf("%.5f %.2f %s\n", p, wp, wbuf);  */

	Debug = 0;
	if( Debug ){
	  nconstit = treeConstit(tree, constit, 0);
	  for(k=0; k<=nconstit; k++){
	    p = probSBLM(constit[k], 3);
	    printf("%.5f %f : %s\n", p, exp(p), constit[k]);
	  }
	}
	freeTree(tree);
      }

      c = fgetc(f);
      while( (c=='\n' || c=='\t' || c==' ') )
	c = fgetc(f);
      
      if( c=='=' ){ /* for Nbest lists marker */
	while( c!='\n' && c!=-1 ){
	  c = fgetc(f);
	}
	while( c=='\n' || c=='\t' || c==' ')
	  c = fgetc(f);
	nt++; i=0;
	/* printf("==%d==\n", nt); */
	flagPPS = PPS;
	cnt++;
      }
    }
    
    fprintf(stderr, "Sentences scored: %d\n", nt);

    if( f!=stdin )
      break;
    if( nt==0 )
      break;
  }
  return 0;
}


int treeConstit(Tree *tree, char constit[][LLEN], int n)
{
  Tree *t;
  char lmod[LLEN], rmod[LLEN], mod[LLEN], lex[SLEN];

  if( !tree || !tree->child ) 
    return n-1;

  strcpy(lmod, "");
  for(t=tree->headchild->siblingL; t!=NULL; t=t->siblingL){
    getLex(lex, t->lex, 0);
    sprintf(mod, "%s~%s~%s~%d ", t->synT, t->preT, lex, t->cv);
    strcat(lmod, mod);
  }

  strcpy(rmod, "");
  for(t=tree->headchild->siblingR; t!=NULL; t=t->siblingR){
    getLex(lex, t->lex, 0);
    sprintf(mod, "%s~%s~%s~%d ", t->synT, t->preT, lex, t->cv);
    strcat(rmod, mod);
  }

  getLex(lex, tree->lex, 0);
  sprintf(constit[n], "%s~%s~%s -> %s~%s~%s : %s: %s:", tree->synT, tree->preT, lex, tree->headchild->synT, tree->headchild->preT, lex, lmod, rmod);

  for(t=tree->child; t!=NULL; t=t->siblingR)
    n = treeConstit(t, constit, n+1);

  return n;
}

void writeAPIprobs(Tree *tree)
{
  char cv[2], lmod[100], rmod[100], apis[300], lex[80];
  double p0, p1;
  Tree *t;

  if( tree==NULL ) return;

  p0 = 0;
  for(t=tree->child; t!=NULL; t=t->siblingR){
    writeAPIprobs(t);
    p0 += t->prob;
  }

  if( tree->child ){
    lmod[0]=0;
    for(t=tree->headchild->siblingL; t!=NULL; t=t->siblingL){
      strcat(lmod, t->synT); strcat(lmod, "~");
      strcat(lmod, t->preT); strcat(lmod, "~");
      getLex(lex, t->lex, 0); strcat(lmod, lex); strcat(lmod, "~");
      sprintf(cv, "%d", t->cv);
      strcat(lmod, cv); strcat(lmod, " ");
    }
    rmod[0]=0;
    for(t=tree->headchild->siblingR; t!=NULL; t=t->siblingR){
      strcat(rmod, t->synT); strcat(rmod, "~");
      strcat(rmod, t->preT); strcat(rmod, "~");
      getLex(lex, t->lex, 0); strcat(rmod, lex); strcat(rmod, "~");
      sprintf(cv, "%d", t->cv);
      strcat(rmod, cv); strcat(rmod, " ");
    }
    getLex(lex, tree->headchild->lex, 0);
    sprintf(apis, "%s~%s~%s -> %s~%s~%s : %s: %s:", tree->synT, tree->preT, tree->lex, tree->headchild->synT, tree->headchild->preT, lex, lmod, rmod);
    p1 =  probSBLM(apis, 3);
    printf("%.5f+%.5f=%.5f : %s\n", p1, p0, p1+p0, apis);
  }
}
