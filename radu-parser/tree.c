#include "tree.h"
#include "treelexhead.h"
#include "grammar.h"

extern int allb;

int equivalentLabel(char *synT1, char *synT2)
{
  if( !strcmp(synT1, synT2) ) return 1;
  if( !strcmp(synT1, "S") && !strcmp(synT2, "SG") ) return 1;
  if( !strcmp(synT1, "SG") && !strcmp(synT2, "S") ) return 1;
  return 0;
}


Tree* readTree(FILE *f, int preprocessFlag)
{
  Tree *tree;
  int off = 0;
  char last[200];

  last[0] = 0;
  tree = readTree1(f, 0, 0, &off, last);
  if( !tree ) return 0;

  updateNToken(tree);
  if( preprocessFlag==2 ){ 
    updatePruning(tree);
    /* undoPunct(tree);  */
    updateLexHead(tree); 
  }
  else{
    /* preprocessing steps taken from Bikel; the order of the updates is important */
    updatePruning(tree);
    updateLexHead(tree); /* updateNPBs() and updateProS() need headchild info */
    updateNPBs(tree);
    updateRepairNPs(tree, "NPB");
    /* updateTrace(tree);*/
    updateProS(tree);
    updateRemoveNull(tree); /* done with span length determined by updateLexHead() */
    updatePunct(tree);

    updateLexHead(tree);
    updateComplements(tree);
    updateRepairProS(tree);
  }
  updateCV(tree);
  updateVi(tree);

  return tree;
}

Tree* readTree1(FILE *f, Tree *parent, Tree *siblingL, int *off, char last[])
{
  Tree *t;
  char buf[2000], sembuf[100], lexid[100];
  int i, j, c, n, semflag, rrbflag;

  c = fgetc(f);  
  if( c==' ' || c=='(' ) /* S1 missing in Gold trees */
    strcpy(buf, "S1");
  else
    {
      i = 0;
      buf[i++] = c;
      while( c != ' ' && c!= '\n' )
	{
	  c = fgetc(f);  
	  buf[i++] = c;
	}
      buf[i-1] = 0;
      c = fgetc(f);
    }
  if( !strcmp(buf, "TOP") )
    strcpy(buf, "S1");

  if( (t = (Tree *)malloc(sizeof(Tree)))==NULL )
    { fprintf(stderr, "Cannot allocate in readTree()\n"); exit(4); }
  else
    allb += sizeof(Tree);
  t->prob = 0;
  t->cv = 0;
  t->lvi = 0; t->rvi = 0;

  t->parent = parent;
  n = strlen(buf);
  j = 0;
  if( strcmp(buf, "-NONE-") && strcmp(buf, "-LRB-") && strcmp(buf, "-RRB-") )
    for(i=1,semflag=0; i<=n && strcmp(buf, "-NONE-"); i++){
      if( semflag ){ sembuf[j++] = buf[i]; }
      if( (buf[i] == '-') || (buf[i] == '=') ){ 
	if( !(i==strlen(buf)-2 && buf[i+1] == 'C') ){ /* keep -C in synT */
	  if( !semflag ){ buf[i] = 0; semflag = 1; }
	  else{ sembuf[j-1] = 0; semflag = 0; }
	}
      }
      if( (buf[i] == '|') ){ buf[i] = 0; } /* only first alternative */  
    }
  sembuf[j] = 0;
  if( buf[0]=='(' )
    for(i=1; i<=strlen(buf); i++)
      buf[i-1] = buf[i];
  strcpy(t->synT, buf);
  if( sembuf ){ strcpy(t->semT, sembuf); }
  else t->semT[0] = 0;
  t->lex[0] = 0;
 
  while( c=='\n' || c=='\t' || c=='\r' || c==' ')
    c = fgetc(f);
  if( !strcmp(buf, "S1") && c==')' ) /* parser output (TOP ) */
    return 0;

  if( c=='(' && strcmp(buf, "-LRB-") )
    {
      t->child = readTree1(f, t, 0, off, last);
      if( !strcmp(t->synT, "S1") || !strcmp(t->synT, "TOP") )
	{ t->siblingL = t->siblingR = 0; return t; }
      c = fgetc(f);
      while( c=='\n' || c=='\t' || c=='\r' || c==' ')
	c = fgetc(f);
      if( c=='(' )
	t->siblingR = readTree1(f, t->parent, t, off, last);
      else
	t->siblingR = NULL;
    }
  else
    {
      i = 0; 
      while( c=='\n' || c=='\t' || c=='\r' || c==' ')
	c = fgetc(f);
      rrbflag = 0;
      while( c!=' ' && ( c!=')' || (!strcmp(t->synT,"-RRB-") && !rrbflag) ) ){
	buf[i++] = c;
	if( c==')' && !strcmp(t->synT,"-RRB-") )
	  rrbflag = 1;
	c = fgetc(f);
      }
      while( c=='\n' || c=='\t' || c=='\r' || c==' ')
	c = fgetc(f);
      buf[i] = 0;
      if( c!=')' )
	assert(c==')'); /* reading at end of leaf */ 
      t->child = 0;
      t->b = *off;
      if( increaseLeavesCount(t->synT, buf, last) )
	*off += 1;
      t->e = *off;
      /*
      if( !strcmp(buf, "-LRB-") )
	strcpy(buf, "(");
      if( !strcmp(buf, "-RRB-") )
	strcpy(buf, ")");
      if( !strcmp(buf, "-LCB-") )
	strcpy(buf, "{");
      if( !strcmp(buf, "-RCB-") )
	strcpy(buf, "}");
	*/
      sprintf(lexid, "[%d-%d]", t->b, t->e);
      strcat(buf, lexid);
      strcpy(t->lex, buf);
      strcpy(t->preT, t->synT); /* preT is the same as synT for leaves */
      strcpy(last, t->lex); /* last is needed for double-quote mismatch */
      
      if( baseVerb(t->synT) )
	t->cv = 1;

      c = fgetc(f);
      while( c=='\n' || c=='\t' || c=='\r' || c==' ')
	c = fgetc(f);
      if( c==')' )
	t->siblingR = 0;
      else
	{
	  while( c=='\n' || c=='\t' || c=='\r' || c==' ')
	    c = fgetc(f);
	  if( c=='(' )
	      t->siblingR = readTree1(f, t->parent, t, off, last);
	}
    }
  
  t->siblingL = siblingL;
  return t;
}

Tree* readCTree(FILE *f)
{
  Tree *tree;
  int off=0;

  tree = readCTree1(f, 0, &off);
  if( !tree ) return 0;
  updatePunct(tree); updatePreT(tree);

  updateNToken(tree);
  updateCV(tree);
  updateVi(tree);
		    
  return tree;
}

Tree* readCTree1(FILE *f, Tree *parent, int *off)
{
  Tree *t, *ct=NULL, *pt;

  int c, idx, hidx;

  t = readCNode(f, &hidx);
  t->parent = parent;

  c = fgetc(f);
  while( c=='\n' || c=='\t' || c=='\r' || c==' ')
    c = fgetc(f);

  idx = 0; pt = 0;
  while( c!=')' ){
    if( c=='(' )
      ct = readCTree1(f, t, off);
    else
      ct = readCLeaf(f, t, c, off);
    idx++;
    if( idx==1 ) {
        assert(ct);
        t->child = ct;
    }    
    if( hidx==idx )
      t->headchild = ct;
    if( !strcmp(t->synT, "S1") || !strcmp(t->synT, "TOP") )
      { t->siblingL = t->siblingR = 0; return t; }
    ct->siblingL = pt;
    pt = ct;
    c = fgetc(f);
    while( c=='\n' || c=='\t' || c=='\r' || c==' ')
      c = fgetc(f);
  }
  /* assign the siblingR links */
  while( ct!=NULL ){
    if( ct->siblingL )
      ct->siblingL->siblingR = ct;
    ct = ct->siblingL;
  }
  return t;
}

Tree* readCNode(FILE *f, int *hidx)
{
  Tree *t;
  char buf[2000];
  int i, c;

  c = fgetc(f);
  i = 0;
  buf[i++] = c;
  while( c!='~' ){
    c = fgetc(f);  
    buf[i++] = c;
  }
  buf[i-1] = 0;
  if( (t = (Tree *)malloc(sizeof(Tree)))==NULL )
    { fprintf(stderr, "Cannot allocate in readTree()\n"); exit(4); }
  else
    allb += sizeof(Tree);
  t->prob = 0;
  t->cv = 0;
  t->lvi = 0; t->rvi = 0;

  if( !strcmp(buf, "TOP") )
    strcpy(buf, "S1");
  
  if( strlen(buf)>1 )
    if( buf[strlen(buf)-1]=='A' && buf[strlen(buf)-2]=='-' )
      buf[strlen(buf)-1] = 'C';
  strcpy(t->synT, buf);
  t->semT[0] = 0;
  t->lex[0] = 0;
  t->headchild = NULL;

  c = fgetc(f);  
  i = 0;
  buf[i++] = c;
  while( c!='~' ){ /* head word */
    c = fgetc(f);  
    buf[i++] = c;
  }
  buf[i-1] = 0;
  strcpy(t->lex, buf);

  c = fgetc(f);  
  while( c!='~' ) /* # children (modulo punc) */
    c = fgetc(f);  

  c = fgetc(f); 
  i = 0;
  buf[i++] = c;
  while( c!=' ' ){ /* head index */
    c = fgetc(f);  
    buf[i++] = c;
  }
  buf[i-1] = 0;
  *hidx = atoi(buf);

  return t;
}

Tree* readCLeaf(FILE *f, Tree *parent, char c, int *off)
{
  Tree *t;
  char buf[2000], lexid[100];
  int i;

  if( (t = (Tree *)malloc(sizeof(Tree)))==NULL )
    { fprintf(stderr, "Cannot allocate in readTree()\n"); exit(4); }
  else
    allb += sizeof(Tree);
  t->prob = 0;
  t->cv = 0;
  t->lvi = 0; t->rvi = 0;
  t->parent = t;
  
  i = 0; 
  buf[i++] = c;
  while( (c=fgetc(f))!='/' )
    buf[i++] = c;
  buf[i] = 0;
  
  t->child = 0;
  t->b = (*off)++;
  t->e = *off;
  sprintf(lexid, "[%d-%d]", t->b, t->e);
  strcat(buf, lexid);
  strcpy(t->lex, buf);
  
  c=fgetc(f);
  i = 0; 
  buf[i++] = c;
  while( (c=fgetc(f))!=' ' )
    buf[i++] = c;
  buf[i] = 0;
  if( !strcmp(buf, "PUNC,") ) strcpy(buf, ",");
  if( !strcmp(buf, "PUNC:") ) strcpy(buf, ":");
  if( !strcmp(buf, "PUNC.") ) strcpy(buf, ".");
  strcpy(t->synT, buf);
  strcpy(t->preT, buf);
  t->semT[0] = 0;

  return t;
}

Tree* readFullTree(FILE *f, int mttokFlag, int *bug)
{
  Tree *tree;
  int offset = 0;
  char last[200];

  *bug = 0;
  tree = readFullTree1(f, 0, 0, &offset, last, mttokFlag, bug);
  if( !tree ) return 0;
  updateCV(tree);
  updateVi(tree);

  return tree;
}


Tree* readFullTree1(FILE *f, Tree *parent, Tree *siblingL, int *off, char last[], int mttokFlag, int *bug)
{
  Tree *t, *tt;
  char buf[2000], lex[100], lexid[100];
  int i, c, nchildren, headindex, idx;

  c = fgetc(f);  
  if( c=='\n' ) /* after reading 0 in case of failed parse */
    return 0;
  i = 0;
  buf[i++] = c;
  while( c != ' ' && c!= '\n' && c!= '~' ){
    c = fgetc(f);  
    buf[i++] = c;
  }
  buf[i-1] = 0;

  if( (t = (Tree *)malloc(sizeof(Tree)))==NULL )
    { fprintf(stderr, "Cannot allocate in readTree(): allb = %d\n", allb); exit(4); }
  else
    allb += sizeof(Tree);
  t->parent = parent;
  strcpy(t->synT, buf);
  t->cv = 0;

  /* guard against reading trees containing some bug due to parsing  
  if( getLabelIndex(t->synT, 0)<0 ) 
    *bug = 1;
    */
  nchildren = 0; headindex = -1;

  if( c== '~' ){
    i=0; c = fgetc(f); buf[i++] = c; 
    while( c!='~' ){ c = fgetc(f);  buf[i++] = c; }
    buf[i-1] = 0;
    nchildren = atoi(buf);
  }

  if( c== '~' ){
    i=0; c = fgetc(f); buf[i++] = c; 
    while( c!=' ' ){ c = fgetc(f);  buf[i++] = c; }
    buf[i-1] = 0;
    headindex = atoi(buf);

    fscanf(f, "%lf", &(t->prob));
  }
  else{
    t->prob = 0;
    if( !strcmp(t->synT, "TOP") ){ /* (TOP RaduErrorParse/NN) in case of failed parse */
      fscanf(f, "%s", buf);
      if( strcmp(buf, "RaduErrorParse/NN)") )
	{ fprintf(stderr, "Error: failed-parse mis-reading\n"); exit(26); }
      return 0;
    }
  }

  t->semT[0] = 0;
  t->lex[0] = 0;
  
  c = fgetc(f);
  while( c=='\n' || c=='\t' || c=='\r' || c==' ')
    c = fgetc(f);
  if( c=='(' && strcmp(buf, "-LRB-") )
    {
      t->child = readFullTree1(f, t, 0, off, last, mttokFlag, bug);

      if( !strcmp(t->synT, "S1") || !strcmp(t->synT, "TOP") ){
	strcpy(t->synT, "S1");
	t->siblingR = 0; 
      }
      else{
	c = fgetc(f);
	while( c=='\n' || c=='\t' || c=='\r' || c==' ')
	  c = fgetc(f);
	if( c=='(' ){
	  t->siblingR = readFullTree1(f, t->parent, t, off, last, mttokFlag, bug);
	}
	else
	  t->siblingR = NULL;
      }
    }
  else{ /* leaf */
    i = 0; 
    if( mttokFlag )
      if( c >= 'A' && c <= 'Z' ){ c = 'a' + c - 'A'; } /* lower case */
    buf[i++] = c;
    while( (c=fgetc(f))!=')' ){
      if( mttokFlag )
	if( c >= 'A' && c <= 'Z' ){ c = 'a' + c - 'A'; } /* lower case */
      buf[i++] = c;
    }
    buf[i] = 0;
    t->child = 0;
    t->b = *off;
    if( increaseLeavesCount(t->synT, buf, last) )
      *off += 1;
    t->e = *off;
    sprintf(lexid, "[%d-%d]", t->b, t->e);
    strcat(buf, lexid);
    strcpy(t->lex, buf);
    strcpy(t->preT, t->synT); /* preT is the same as synT for leaves */
    strcpy(last, t->lex); /* last is needed for double-quote mismatch */

    getLex(lex, t->lex, 0);

    /* guard against reading trees containing some bug due to parsing and/or tagging noise */  
    if( !strcmp(t->preT, ",") && strcmp(lex, ",") )
      *bug = 1;
    if( !strcmp(t->preT, ":") && 
	(strcmp(lex, "'") && strcmp(lex, ";") && strcmp(lex, ":") && strcmp(lex, "-") && strcmp(lex, "--") && strcmp(lex, "...")) )
      *bug = 1;

    if( baseVerb(t->synT) )
      t->cv = 1;
    
    c = fgetc(f);
    while( c=='\n' || c=='\t' || c=='\r' || c==' ')
      c = fgetc(f);
    if( c==')' )
      t->siblingR = 0;
    else{
      while( c=='\n' || c=='\t' || c=='\r' || c==' ')
	c = fgetc(f);
      if( c=='(' ){
	t->siblingR = readFullTree1(f, t->parent, t, off, last, mttokFlag, bug);
      }
    }
  }
  t->siblingL = siblingL;
  
  for(tt=t->child, idx=1; tt!=NULL; tt=tt->siblingR){
    if( idx==headindex ){
      t->headchild = tt;
      getLex(lex, tt->lex, 0);
      strcpy(t->preT, tt->preT); 
      strcpy(t->lex, lex);
    }
    if( strcmp(tt->synT, ",") && strcmp(tt->synT, ":") &&
	strcmp(tt->synT, "``") && strcmp(tt->synT, "''") && strcmp(tt->synT, ".") )
      idx++;
  }
  if( idx-1!=nchildren && nchildren && 0 )
    { fprintf(stderr, "Warning: # children does not correspond (%d vs. %d)\n", nchildren, idx-1); }

  return t;
}

void convertPTBTree2MTTree(Tree *tree)
{
  Tree *t;

  if( !tree ) return;

  if( !tree->child )
    convertPTBLex2MTLex(tree);
  else
    for(t=tree->child; t!=NULL; t=t->siblingR)
      convertPTBTree2MTTree(t);
}

void convertPTBLex2MTLex(Tree *tree) /* keep in sync with writeMTLex(Edge*) in edge.c */
{
  char lex[200], piece[MAXPIECE][80], span[80];
  int b, e, i, j, n, npiece, flag;
  Tree *ctree, *ptree;

  getLex(lex, tree->lex, 1); /* lower-case-ing */

  b = getBLex(tree->lex);
  e = getELex(tree->lex);
  if( b>=0 && e>=0 )
    sprintf(span, "[%d-%d]", b, e);
  else
    strcpy(span, "");

  if( !strcmp(lex, "--") && b==0 && e==1 ){ /* initial -- <-> * */
    sprintf(tree->lex, "*%s", span);
  }
  else if( !strcmp(lex, "''") ){ /* '' <-> " (loseless because POS is preserved as '') */
    sprintf(tree->lex, "\"%s", span);
  }
  else if( !strcmp(lex, "``") ){ /* `` <-> " (loseless because POS is preserved as ``) */
    sprintf(tree->lex, "\"%s", span);
  }
  else if( !strcmp(lex, "-LRB-") || !strcmp(lex, "-lrb-")){ /* -LRB- <-> (  */
    sprintf(tree->lex, "(%s", span);
  }
  else if( !strcmp(lex, "-RRB-") || !strcmp(lex, "-rrb-") ){ /* -RRB- <-> )  */
    sprintf(tree->lex, ")%s", span);
  }
  else if( !strcmp(lex, "-LCB-") || !strcmp(lex, "-lcb-") ){ /* -LCB- <-> {  */
    sprintf(tree->lex, "{%s", span);
  }
  else if( !strcmp(lex, "-RCB-") || !strcmp(lex, "-rcb-") ){ /* -RCB- <-> }  */
    sprintf(tree->lex, "}%s", span);
  }  
  else
    sprintf(tree->lex, "%s%s", lex, span);

  n = strlen(lex);
  npiece = 0;
  if( lex[0] == '-' || lex[n-1] == '-' )
    flag = 1;
  else
    flag = 0;
  for(i=0, j=0; i<n; i++){
    if( lex[i] == '-' && !flag){
      piece[npiece][j] = 0;
      j = 0;
      if( npiece+2 >= MAXPIECE )
	{ fprintf(stderr, "Error: maximum number of pieces %d exceeded for: %s\n", MAXPIECE, tree->lex); exit(24); }
      strcpy(piece[npiece+1], "@-@");
      npiece += 2;
    }
    else
      piece[npiece][j++] = lex[i];
  }
  piece[npiece++][j] = 0;
  if( npiece>1 )
    convertPTBLex2ComplexNT(tree, b, e, piece, npiece);
  
  /* propagating the modified lex up the tree */
  ctree = tree;
  ptree = ctree->parent;
  while( ptree->headchild==ctree ){
    strcpy(ptree->lex, ctree->lex);
    ctree = ptree;
    ptree = ctree->parent;
    if( !ptree ) break;
  } 
}

void convertPTBLex2ComplexNT(Tree *tree, int b, int e, char piece[][80], int npiece)
{
  Tree *t, *pt;
  char newsynT[80], buf[200];
  int i;

  sprintf(newsynT, "C%s", tree->synT);
  strcpy(tree->synT, newsynT);

  pt = 0;
  for(i=0; i<npiece; i++){
    if( (t = (Tree *)malloc(sizeof(Tree)))==NULL )
      { fprintf(stderr, "Cannot allocate in readTree()\n"); exit(4); }
    else
      allb += sizeof(Tree);
    t->prob = 0;
    t->cv = 0;
    t->lvi = 0; t->rvi = 0;
    t->parent = tree;   
    t->child = t->headchild = 0;
    if( i==0 )
      tree->child = t;
    t->siblingL = pt;
    if( pt )
      pt->siblingR = t;

    sprintf(buf, "%s[%d-%d]", piece[i], b, e);
    strcpy(t->lex, buf);
    strcpy(t->synT, tree->preT); 
    strcpy(t->preT, tree->preT);
    t->semT[0] = 0;
    if( baseVerb(t->synT) )
      t->cv = 1;
    
    pt = t;
  } 
  pt->siblingR = 0;

  tree->headchild = pt;
  strcpy(tree->lex, tree->headchild->lex);
}

void updatePreT(Tree *tree)
{
  Tree *t;

  for(t=tree->child; t!=NULL; t=t->siblingR)
    updatePreT(t);

  if( tree->child ){
//    if( !tree->headchild ) printf("");
    assert(tree->headchild);
    assert(tree->headchild->preT);
    strcpy(tree->preT, tree->headchild->preT);
  } 
}

void updatePruning(Tree *tree)
{
  Tree *t;
 
  if( !tree ) return;

  for(t=tree->child; t!=NULL; t=t->siblingR)
    updatePruning(t);

  /* remove `` and '' and . from the tree */
  if( tree->child==NULL ){
    if( !strcmp(tree->synT, "``") || !strcmp(tree->synT, "''") || !strcmp(tree->synT, ".") ){
      if( tree->parent->child == tree ) 
	if( tree->siblingR )
	  tree->parent->child = tree->siblingR;
	else
	  tree->parent->child = tree->child;
      if( tree->siblingR ) 
	tree->siblingR->siblingL = tree->siblingL;
      if( tree->siblingL ) 
	tree->siblingL->siblingR = tree->siblingR;
    
      free(tree); allb -= 1;
      return;
    }
  }
}

int updateNPBs(Tree *tree)
{
  Tree *t, *tt, *newt, *pcoord, *lcoord, *rcoord; 
  char buf[10];
  int flag, addNP, a, action;

  a = 0;
  flag = 1;
  if( !strcmp(tree->synT, "NP") ){
    for(t=tree->child; t!=NULL; t=t->siblingR){
      if( dominatesSynTNP(t) ){
	if( (!strcmp(t->synT, "ADJP")||!strcmp(t->synT,"NAC")) && t->siblingR ){
	  ; /* flag stays 1 */
	}
	else
	  { flag = 0; break; }
      }
    }

    if( flag==0 ){
      if( t->child ){
	for(tt=t->child; tt->siblingR!=NULL; tt=tt->siblingR) ;
	if(  !strcmp(tt->synT, "POS") ) 
	  flag = 1;
      }
    }
     
    if( flag ){ 
      addNP = 0;
      if( strcmp(tree->parent->synT, "NP") && strcmp(tree->parent->synT, "NPB") )
	addNP = 1;
      else{
	if( !strcmp(tree->parent->synT, "NPB") ){ 
	  for(t=tree; t->siblingR!=NULL; t=t->siblingR) ;
	  if( t->synT[0]=='S' )
	    addNP = 1;
	}
	else{
	  if( !strcmp(tree->parent->synT, "NP") ){
	    if( coordinatedPhrase(tree->parent, &pcoord, &lcoord, &rcoord) ){
	      addNP = 1;
	    }
	    for(t=tree->parent->child; t!=NULL; t=t->siblingR )
	      if( t->synT[0]=='N' ) break;
	    if( t != tree ) 
	      addNP = 1;
	    if( !strcmp(tree->synT, "NX") )
	      addNP = 1; 
	  }
	}
      }
      
      if( addNP ){
	newt = createSingleNode("NPB", tree);
	newt->child = tree->child;
	newt->headchild = tree->headchild;
	newt->parent = tree;
	for(t=tree->child; t!=NULL; t=t->siblingR)
	  t->parent = newt;
	tree->child = newt;
	tree->headchild = newt;
      }
      else{ /* just replace NP with NPB, no extra node added */
	strcpy(buf, "NPB");
	strcpy(tree->synT, buf);
      }
    }
  }
  if( preTNoun(tree->synT) && addNPB(tree->parent->synT) && 0 ){ /* addNPB() is out of hand */ 
    for(t=tree->siblingL; t!=NULL; t=t->siblingL)
      if( !preTNoun(t->synT) && strcmp(t->synT, "CC") ) break;
    if( t==NULL ) a = 1;
    else
      if( strcmp(t->synT, "NPB") ) a = 1;
    if( a ){ /* missed NN-s */
      newt = createSingleNode("NPB", tree);
      newt->child = tree;
      newt->headchild = tree;
      newt->parent = tree->parent;
      if( tree->parent->child==tree ) tree->parent->child = newt;
      tree->parent = newt;
      newt->siblingL = tree->siblingL; tree->siblingL = 0;
      newt->siblingR = tree->siblingR; tree->siblingR = 0;
      if( newt->siblingL ) newt->siblingL->siblingR = newt;
      if( newt->siblingR ) newt->siblingR->siblingL = newt;
      a = 1; /* must climb up to the new NPB to continue the traversal */
    }
  }
    
  /*pre-order tree traversal */
  for(t=tree->child; t!=NULL; t=t->siblingR){
    action = updateNPBs(t);
    if( action==1 ) 
      t = t->parent;
  }

  return a;
}

Tree *createSingleNode(char *label, Tree *tree)
{
  Tree *newt; 

  if( (newt = (Tree *)malloc(sizeof(Tree)))==NULL )
    { fprintf(stderr, "Cannot allocate in readTree()\n"); exit(4); }
  else
    allb += sizeof(Tree);

  strcpy(newt->synT, label);
  strcpy(newt->preT, tree->preT);
  strcpy(newt->semT, tree->semT);
  strcpy(newt->lex, tree->lex);
  newt->b = tree->b; newt->e = tree->e; 
  newt->siblingL = 0;
  newt->siblingR = 0;
  newt->prob = 0;
  newt->parent = 0;
  newt->lvi = 0; newt->rvi = 0;

  return newt;
}

void updateRepairNPs(Tree *tree, char *label)
{
  Tree *t;

  if( !strcmp(tree->synT, label) ){
    for(t=tree->child; t->siblingR!=NULL; t=t->siblingR) ;
    if( (!strcmp(t->synT, "S") || !strcmp(t->synT, "SBAR") || !strcmp(t->synT, "SBARQ") ||
	!strcmp(t->synT, "SINV") || !strcmp(t->synT, "SQ")) && 
	t->siblingL && width(t)>0 && strcmp(t->siblingL->synT, label) ){
      if( strcmp(tree->parent->synT, "NP") && strcmp(label, "NP") )
	fprintf(stderr, "Error: parent of NPB is not NP but %s\n", tree->parent->synT);
      
      if( tree->siblingR )
	{ t->siblingR = tree->siblingR; t->siblingR->siblingL = t; }
      tree->siblingR = t;
      if( t->siblingL )
	t->siblingL->siblingR = NULL;
      t->siblingL = tree;
      t->parent = tree->parent;
    }
  } 

  /*pre-order tree traversal */
  for(t=tree->child; t!=NULL; t=t->siblingR)
    updateRepairNPs(t, label);
}

void updateTrace(Tree *tree)
{
  Tree *t;
  char buf[100];

  for(t=tree->child; t!=NULL; t=t->siblingR)
    updateTrace(t);

  if( !strcmp(tree->synT,"-NONE-") && tree->lex[0]=='*' && tree->lex[1]=='T' ){
    free(tree->synT);
    strcpy(buf, "TRACE");
    strcpy(tree->synT, buf);
    strcpy(tree->preT, buf);
  }

}


void updateProS(Tree *tree)
{
  Tree *t, *tt, *pcoord, *lcoord, *rcoord; 
  char buf[1000];

  for(t=tree->child; t!=NULL; t=t->siblingR)
    updateProS(t);

  if( !strcmp(tree->synT, "S")  ){
    if( !strcmp(tree->headchild->synT, "VP") ){
      for(t=tree->child; t!=NULL; t=t->siblingR){
	if( strlen(t->semT) >= 3 ){
	  if( t->semT[0] == 'S' && t->semT[1] == 'B' && t->semT[2] == 'J' ){
	    for(tt=t; tt->child!=NULL; tt=tt->child ) ;
	    if( !strcmp(tt->synT, "-NONE-") ){ /* if t dominates a NULL child */
	      /* rename S into SG */
	      strcpy(buf, "SG"); strcpy(tree->synT, buf); return;
	    }
	  }
	}
      }
      for(t=tree->child; t!=NULL; t=t->siblingR)
	if( !(spuriousLabel(t->synT) || !strcmp(t->synT, "CC")) ) break;
      if( t==tree->headchild ){ /* undocumented; see WSJ_0254.MRG/24 */
	/* rename S into SG */
	strcpy(buf, "SG"); strcpy(tree->synT, buf); return; 
      }
    }
    if( !strcmp(tree->headchild->synT, "SG") ){ /*undocumented; see WSJ_0436.MRG/30 */
      if( coordinatedPhrase(tree, &pcoord, &lcoord, &rcoord) ){
	if( !strcmp(pcoord->synT, "CC") && lcoord==tree->headchild ){
	  /* rename S into SG */
	  strcpy(buf, "SG"); strcpy(tree->synT, buf); return;
	}
      }
    }
  }

}

void updateRemoveNull(Tree *tree)
{
  Tree *t;

  for(t=tree->child; t!=NULL; t=t->siblingR)
    updateRemoveNull(t);
  
  if( tree->b==tree->e && !strcmp(tree->preT, "-NONE-") ){
    if( tree->siblingL==NULL ){
      tree->parent->child = tree->parent->child->siblingR;
      if( tree->parent->child )
	tree->parent->child->siblingL = 0;
    }
    else{
      tree->siblingL->siblingR = tree->siblingR;
      if( tree->siblingR )
	tree->siblingR->siblingL = tree->siblingL;
    }
  }
}


void updatePunct(Tree *tree)
{
  Tree *t, *tmove, *tsiblingL, *tsiblingR, *tchild;

  for(t=tree->child; t!=NULL; t=t->siblingR)
    updatePunct(t);

  if( tree->child && tree->parent ){ 
    /* move end punctation up the tree */
    for(t=tree->child; t->siblingR!=NULL; t=t->siblingR) ;
    if( (!strcmp(t->synT, ",") || !strcmp(t->synT, ":")) && t->siblingL ){
      tmove = t;
      if( tmove->siblingL ){ tmove->siblingL->siblingR = 0; }  /* cut out */
      for(t=tree; t && t->siblingR==NULL; t=t->parent) ; /* go up until siblingR */
      if( t ){
	tsiblingL = t;
	tmove->parent = tsiblingL->parent;    /* link back to parent */
	tmove->siblingR = tsiblingL->siblingR;/* link to siblingR */
	if( tsiblingL->siblingR ){ tsiblingL->siblingR->siblingL = tmove; }
	tsiblingL->siblingR = tmove;
	tmove->siblingL = tsiblingL;
      }
    }

    /* move start punctation up the tree -- for PRN especially */
    if( (!strcmp(tree->child->synT, ",") || !strcmp(tree->child->synT, ":")) ){
      tmove = tree->child; tchild = tmove->siblingR;
      if( (!strcmp(tmove->synT, ",") || !strcmp(tmove->synT, ":")) ){
	for(t=tree; t && t->siblingL==NULL && strcmp(t->synT, "S1") ; t=t->parent) ; /* go up until siblingL */
	if( t && strcmp(t->synT, "S1") ){
	  tsiblingR = t;
	  tmove->parent = tsiblingR->parent;    /* link back to parent */
	  tmove->siblingL = tsiblingR->siblingL;/* link to siblingL */
	  if( tsiblingR->siblingL ){ tsiblingR->siblingL->siblingR = tmove; }
	  tsiblingR->siblingL = tmove;
	  tmove->siblingR = tsiblingR;
	}
      }
      tree->child = tchild;
    }
  }

}


void updateComplements(Tree *tree)
{
  Tree *t, *pcoord, *lcoord, *rcoord;
  int flagH, flagC, coord;
  char buf[100], *semTag;

  for(t=tree->child; t!=NULL; t=t->siblingR)
    updateComplements(t);

  if( tree->parent ){
    coord = coordinatedPhrase(tree->parent, &pcoord, &lcoord, &rcoord);

    if( !strcmp(tree->parent->synT, "PP") && !coord ){
      if( !tree->child ) return;
      if( !strcmp(tree->synT, "PRN") || !strcmp(tree->synT, "CONJP") )
	return;
      flagH = 0; flagC = 0; 
      for(t=tree->parent->child; t!=NULL; t=t->siblingR){
	if( t==tree ) break;
	if( t==tree->parent->headchild ) flagH = 1;
	if( flagH && t->synT[strlen(t->synT)-1]=='C' && t->synT[strlen(t->synT)-2]=='-' )
	  if( strcmp(t->synT, "ADVP-C") ) 
	    flagC = 1;
      }
      if( flagC ) return; 
      if( !strcmp(tree->synT, "ADVP") || !strcmp(tree->semT, "ADV") ||
	  !strcmp(tree->synT, "ADJP") || !strcmp(tree->synT, "PP") ){
	if( !flagH ) return;
      }
      strcpy(buf, tree->synT); strcat(buf, "-C"); strcpy(tree->synT, buf);
      return;
    }
    
    semTag = semanticTag(tree);

    if( (!strcmp(tree->parent->synT, "S") || !strcmp(tree->parent->synT, "SG")) &&
	!semTag && !coord &&
	strcmp(tree->parent->headchild->synT, "SG") ){ /* undocumented; see WSJ_0231.MRG/15 */
      if( !strcmp(tree->synT, "NP") || !strcmp(tree->synT, "SBAR") || 
	  !strcmp(tree->synT, "S") || !strcmp(tree->synT, "SG") ){
	strcpy(buf, tree->synT); strcat(buf, "-C"); strcpy(tree->synT, buf);
	return;
      }
    }
    if( !strcmp(tree->parent->synT, "VP") && !semTag && !coord ){
      flagH = 0; flagC = 0; 
      for(t=tree->parent->child; t!=NULL; t=t->siblingR){
	if( t==tree ) break;
	if( t==tree->parent->headchild ) flagH = 1;
	if( flagH && t->synT[strlen(t->synT)-1]=='C' && t->synT[strlen(t->synT)-2]=='-' )
	  if( strcmp(t->synT, "ADVP-C") ) 
	    flagC = 1;
      }
      if( flagC ) return; 
      if( !strcmp(tree->synT, "NP") || !strcmp(tree->synT, "SBAR") || 
	  !strcmp(tree->synT, "S") || !strcmp(tree->synT, "SG") ){
	strcpy(buf, tree->synT); strcat(buf, "-C"); strcpy(tree->synT, buf);
	return;
      }
      if( !strcmp(tree->synT, "VP") ){
	for(t=tree->siblingL; t!=NULL; t=t->siblingL)
	  if( strcmp(t->synT, "RB") && strcmp(t->synT, "ADVP") ) break;
	if( t ){
	  strcpy(buf, tree->synT); strcat(buf, "-C"); strcpy(tree->synT, buf);
	  return;
	}
      }
    }
    if( !strcmp(tree->parent->synT, "SBAR") ){ /* SBAR seem not to care about semTag and coord */
      if( !strcmp(tree->synT, "S") || !strcmp(tree->synT, "SG") ){
	strcpy(buf, tree->synT); strcat(buf, "-C"); strcpy(tree->synT, buf);
	return;
      }
      if( !strcmp(tree->synT, "NP") ){ /* undocumented: see WSJ_0261.MRG/7 WSJ_0331.MRG/27 */
	for(t=tree->siblingL; t!=NULL; t=t->siblingL)
	  if( !strcmp(t->synT, "WHNP") || !strcmp(t->synT, "WP") ) break;
	if( t ){
	  strcpy(buf, tree->synT); strcat(buf, "-C"); strcpy(tree->synT, buf);
	  return;
	}
      }
    }
  }
}

void updateRepairProS(Tree *tree)
{
  Tree *t;

  for(t=tree->child; t!=NULL; t=t->siblingR)
    updateRepairProS(t);

  if( !strcmp(tree->synT, "SG")  ){
    for(t=tree->headchild; t!=NULL; t=t->siblingL){
      if( t->synT[strlen(t->synT)-1]=='C' &&  t->synT[strlen(t->synT)-2]=='-' ){
	tree->synT[strlen(tree->synT)-1]  = 0; break;
      }
    }
  }
}

int updateNToken(Tree *tree)
{
  Tree *t;
  int c;

  c = 0;
  for(t=tree->child; t!=NULL; t=t->siblingR)
    c += updateNToken(t);
  tree->ntoken = c+1;

  return tree->ntoken;
}

void postprocessTree(Tree *tree)
{
  if( !tree ) return;

  undoComplements(tree);
  /* undoPunct(tree); does not count for evalb */
  undoProS(tree);
  undoRepairNPBs(tree);
  undoNPBs(tree);
}

void undoComplements(Tree *tree)
{
  Tree *t;
  char buf[1000];

  for(t=tree->child; t!=NULL; t=t->siblingR)
    undoComplements(t);
  
  if( tree->synT[strlen(tree->synT)-1] == 'C' && tree->synT[strlen(tree->synT)-2] == '-' ){
    strcpy(buf, tree->synT); buf[strlen(buf)-2] = 0;
    strcpy(tree->synT, buf);
  }
}

void undoPunct(Tree *tree)
{
  Tree *t, *tt, *p, *punct;

  for(t=tree->child; t!=NULL; t=t->siblingR)
    undoPunct(t);

  if( !tree->child ) return;

  for(t=tree->child; t!=NULL; t=t->siblingR){
    if( !t->siblingL ) continue;
    if( (!strcmp(t->siblingL->synT, ",") || !strcmp(t->siblingL->synT, ":")) && 
	t->siblingL->siblingL ){
      punct = t->siblingL;
      tt = punct->siblingL;
      p = tt;
      while( p->child ){
	for(tt=p->child; tt->siblingR!=NULL; tt=tt->siblingR) ;
	p = tt;
      }
      if( !tt->siblingR ){ /* bring the punct down to the right of tt */
	p = tt->parent;

	punct->siblingL->siblingR = t;
	t->siblingL = punct->siblingL;

	tt->siblingR = punct;
	punct->siblingL = tt;
	punct->siblingR = NULL;
	punct->parent = p;
      }
    }
  }
}

void undoProS(Tree *tree)
{
  Tree *t;
  char buf[100];

  for(t=tree->child; t!=NULL; t=t->siblingR)
    undoProS(t);

  if( !strcmp(tree->synT, "SG") ){
    strcpy(buf, "S");
    strcpy(tree->synT, buf);
  }


}

void undoRepairNPBs(Tree *tree)
{

}

void undoNPBs(Tree *tree)
{
  Tree *t, *tt;
  int flag;

  flag = 0;
  if( !strcmp(tree->synT, "NP") )
    for(t=tree->child; t!=NULL; t=t->siblingR)
      if( !strcmp(t->synT, "NPB") ){
	if( !t->siblingL && !t->siblingR ){ /* newly added NPB; must be removed */
	  if( tree->child==t ){
	    tree->child = t->child;
	    for(tt=t->child; tt!=NULL; tt=tt->siblingR)
	      tt->parent = tree;
	    flag = 1;
	  }
	}
	else{ /* just transform the node into NP */
	  strcpy(t->synT, "NP");
	}
      }

  if( flag && tree->parent ) tree = tree->parent;
  for(t=tree->child; t!=NULL; t=t->siblingR)
    undoNPBs(t);
}

char* semanticTag(Tree *tree)
{
  char lex[200];

  if( !tree ) return 0;
  if( !strcmp(tree->semT, "ADV") || !strcmp(tree->semT, "VOC") || !strcmp(tree->semT, "BNF") ||
      !strcmp(tree->semT, "DIR") || !strcmp(tree->semT, "EXT") || !strcmp(tree->semT, "LOC") || 
      !strcmp(tree->semT, "MNR") || !strcmp(tree->semT, "TMP") || !strcmp(tree->semT, "CLR") || 
      !strcmp(tree->semT, "PRP") )
    return tree->semT;
  if( tree->child ){
    getLex(lex, tree->child->lex, 0);
    if( !strcmp(lex, "because") )
      return "because";
  }
  return 0;
}

int findLeafVerb(Tree *tree)
{
  Tree *t;

  if( !tree->child){
    if( strlen(tree->synT)>1 )
      if( tree->synT[0] == 'V' && tree->synT[1] == 'B' )
	return 1;
  }
  else{
    if( !strcmp(tree->synT, "NPB") )
      return 0;
    for(t=tree->child; t!=NULL; t=t->siblingR)
      if( findLeafVerb(t) )
	return 1;
  }
  return 0;
}

void findVBinNPB(Tree *tree)
{
  Tree *t;

  for(t=tree->child; t!=NULL; t=t->siblingR)
    findVBinNPB(t);

  if( !strcmp(tree->synT, "NPB") )
      findVBinNPBRec(tree);
}

int baseVerb(char *label)
{
  if( !strcmp(label, "VB") || !strcmp(label, "VBD") || !strcmp(label, "VBG") || 
      !strcmp(label, "VBN") || !strcmp(label, "VBP") || !strcmp(label, "VBZ") )
    return 1;
  return 0;
}
   
void findVBinNPBRec(Tree *tree)
{
  Tree *t;

  for(t=tree->child; t!=NULL; t=t->siblingR)
    findVBinNPBRec(t);

  if( tree->child==NULL && baseVerb(tree->synT) )
      fprintf(stderr, "%s(%s)\n", tree->synT, tree->lex);
}


int increaseLeavesCount(char *synT, char *lex, char *last)
{
  if( !strcmp(synT, "-NONE-") )
    return 0;

 if( !strcmp(lex, "'") && !strcmp(last, "'") )
   return 0;

 if( !strcmp(lex, "`") && !strcmp(last, "`") )
   return 0;
     
 return 1;
}

void updateSpans(Tree *tree)
{
  Tree *t;

  if( tree->child==NULL )
    return;

  for(t=tree->child; t!=NULL; t=t->siblingR)
    updateSpans(t);
      
  tree->b = tree->child->b;
  for(t=tree->child; t->siblingR!=NULL; t=t->siblingR)
    ;
  tree->e = t->e;
}

void updateCV(Tree *tree)
{
  Tree *t;

  for(t=tree->child; t!=NULL; t=t->siblingR){
    updateCV(t);
    if( t->cv )
      tree->cv = 1;
  }
  if( !strcmp(tree->synT, "NPB") )
    tree->cv = 0;
  
  /* leaves are already marked for cv */
}

void updateVi(Tree *tree)
{
  Tree *t;

  for(t=tree->child; t!=NULL; t=t->siblingR)
    updateVi(t);
  
  if( !tree->child ) return;

  tree->lvi = tree->headchild->lvi;
  for(t=tree->headchild->siblingL; t!=NULL; t=t->siblingL)
    tree->lvi = tree->lvi || t->lvi || t->rvi || baseVerb(t->preT);

  tree->rvi = tree->headchild->rvi;
  for(t=tree->headchild->siblingR; t!=NULL; t=t->siblingR)
    tree->rvi = tree->rvi || t->lvi || t->rvi || baseVerb(t->preT);
  
}

int getLeaves(Tree *tree, char *leaf[], int i)
{
  Tree *t;

  if( tree==NULL )
    return i;

  if( tree->child==NULL && strcmp(tree->synT, "-NONE-") ){
    leaf[i] = tree->lex;
    return i+1;
  }
  else
    for(t=tree->child; t!=NULL; t=t->siblingR)
      i = getLeaves(t, leaf, i);

  return i;
}

void updateLexHead(Tree *tree)
{
  Tree *t, *newh, *pcoord, *lcoord, *rcoord;
  int ar, coord, h;

  updateSpans(tree);

  if( tree==NULL )
    return;
  if( tree->child==NULL )
    return;

  for(t=tree->child, ar=0; t!=NULL; t=t->siblingR, ar++)
    updateLexHead(t);

  lexHead(tree, tree->lex, tree->preT);
  coord = coordinatedPhrase(tree, &pcoord, &lcoord, &rcoord);      

  if( coord && strcmp(tree->synT, "NPB") ){/* coordinated phrases; not in NPB */
    for(t=tree->child, h=0; t!=NULL; t=t->siblingR, h++){
      if( t==tree->headchild )
	break;
    }

    if( h < ar-2 && coordinatedLabel(t->siblingR, tree, &lcoord, &rcoord) ){
    }
    else{
      if( h >= 2 && coordinatedLabel(t->siblingL, tree, &lcoord, &rcoord) ){
	newh =  t->siblingL->siblingL;
	if( (!strcmp(newh->synT, "CC") || !strcmp(newh->synT, ",") || !strcmp(newh->synT, ":") || !strcmp(newh->synT, "PRN")) && newh->siblingL )
	  newh =  newh->siblingL;
	if( newh==lcoord ){ 
	  tree->headchild = newh;
	  strcpy(tree->lex, newh->lex);
	  strcpy(tree->preT, newh->preT);
	}
      }
    }	
  }
}

int lexHead(Tree *tree, char lex[], char preT[])
{
  int found;
  Tree *tt;

  if( isLabel(tree->synT, "ADJP") )
    return lexSearchLeft(tree, ADJP, nADJP, lex, preT);
      
  if( isLabel(tree->synT, "ADVP") )
    return lexSearchRight(tree, ADVP, nADVP, lex, preT);

  if( isLabel(tree->synT, "CONJP") )
    return lexSearchRight(tree, CONJP, nCONJP, lex, preT);

  if( isLabel(tree->synT, "FRAG") )
    return lexSearchRight(tree, FRAG, nFRAG, lex, preT);

  if( isLabel(tree->synT, "INTJ") )
    return lexSearchLeft(tree, INTJ, nINTJ, lex, preT);

  if( isLabel(tree->synT, "LST") )
    return lexSearchRight(tree, LST, nLST, lex, preT);

  if( isLabel(tree->synT, "NAC") )
    return lexSearchLeft(tree, NAC, nNAC, lex, preT);

  if( isLabel(tree->synT, "PP") )
    return lexSearchRight(tree, PP, nPP, lex, preT);

  if( isLabel(tree->synT, "PRN") )
    return lexSearchLeft(tree, PRN, nPRN, lex, preT);

  if( isLabel(tree->synT, "PRT") )
    return lexSearchRight(tree, PRT, nPRT, lex, preT);

  if( isLabel(tree->synT, "QP") )
    return lexSearchLeft(tree, QP, nQP, lex, preT);

  if( isLabel(tree->synT, "RRC") )
    return lexSearchRight(tree, RRC, nRRC, lex, preT);

  if( isLabel(tree->synT, "S") || isLabel(tree->synT, "SG") )
    return lexSearchLeft(tree, S, nS, lex, preT);

  if( isLabel(tree->synT, "SBAR") )
    return lexSearchLeft(tree, SBAR, nSBAR, lex, preT);

  if( isLabel(tree->synT, "SBARQ") )
    return lexSearchLeft(tree, SBARQ, nSBARQ, lex, preT);

  if( isLabel(tree->synT, "SINV") )
    return lexSearchLeft(tree, SINV, nSINV, lex, preT);

  if( isLabel(tree->synT, "SQ") )
    return lexSearchLeft(tree, SQ, nSQ, lex, preT);

  if( isLabel(tree->synT, "UCP") )
    return lexSearchRight(tree, UCP, nUCP, lex, preT);

  if( isLabel(tree->synT, "VP") )
    return lexSearchLeft(tree, VP, nVP, lex, preT);

  if( isLabel(tree->synT, "WHADJP") )
    return lexSearchLeft(tree, WHADJP, nWHADJP, lex, preT);

  if( isLabel(tree->synT, "WHADVP") )
    return lexSearchRight(tree, WHADVP, nWHADVP, lex, preT);

  if( isLabel(tree->synT, "WHNP") )
    return lexSearchLeft(tree, WHNP, nWHNP, lex, preT);

  if( isLabel(tree->synT, "WHPP") )
    return lexSearchRight(tree, WHPP, nWHPP, lex, preT);

  if( isLabel(tree->synT, "NP") || isLabel(tree->synT, "NPB") )
    {
      for(tt=tree->child; tt->siblingR!=NULL; tt=tt->siblingR) ;
      if( !strcmp(tt->synT, "POS") ){  
	strcpy(lex, tt->lex); strcpy(preT, tt->preT); tree->headchild = tt; return 1; 
      }
      lexSearchRightPar(tree, NP1, nNP1, lex, preT, &found); if( found ) return 1; 
      lexSearchLeftPar(tree, NP2, nNP2, lex, preT, &found);  if( found ) return 1;
      lexSearchRightPar(tree, NP3, nNP3, lex, preT, &found); if( found ) return 1;
      lexSearchRightPar(tree, NP4, nNP4, lex, preT, &found); if( found ) return 1;
      lexSearchRightPar(tree, NP5, nNP5, lex, preT, &found); if( found ) return 1;
      /* here? default rule for right */
      strcpy(lex, tt->lex); strcpy(preT, tt->preT); 
      tree->headchild = tt;
      return 0;
    }

  /* here? default rule */
  strcpy(lex, tree->child->lex); strcpy(preT, tree->child->preT); 
  tree->headchild = tree->child;
  return 0;
}

int isLabel(char* synT, char *label)
{
  char buf[100];
  if( !strcmp(synT, label) )
    return 1;

  strcpy(buf, label);
  strcat(buf, "-C");
  if( !strcmp(synT, buf) )
    return 1;

  return 0;
}

int lexSearchLeft(Tree *tree, char* list[], int nlist, char lex[], char preT[])
{
  Tree *t;
  int i;

  for(i=0; i<nlist; i++)
    for(t=tree->child; t!=NULL; t=t->siblingR)
      if( equivalentLabel(t->synT, list[i]) && t->b < t->e ){
	strcpy(lex, t->lex);
	strcpy(preT, t->preT);
	tree->headchild = t;
	return 1;
      }
  if( nlist==0 ){ /* if empty (PRN), take the left-most non-punc child, if exists; is this ok?*/
    t = tree->child;
    if( (!strcmp(t->synT, ",") || !strcmp(t->synT, ":")) && t->siblingR )
      t = t->siblingR;
    strcpy(lex, t->lex); 
    strcpy(preT, t->preT);
    tree->headchild = t;
    return 1;
  }

  /* here when all tree below is empty */
  strcpy(lex, tree->child->lex); 
  strcpy(preT, tree->child->preT); 
  tree->headchild = tree->child;
  return 0;
}

int lexSearchRight(Tree *tree, char* list[], int nlist, char lex[], char preT[])
{
  Tree *t, *tt;
  int i;

  for(tt=tree->child; tt->siblingR!=NULL; tt=tt->siblingR)
    ;

  for(i=0; i<nlist; i++)
    for(t=tt; t!=NULL; t=t->siblingL)
      if( equivalentLabel(t->synT, list[i]) && t->b < t->e ){
	strcpy(lex, t->lex);
	strcpy(preT, t->preT);
	tree->headchild = t;
	return 1;
      }
  if( !strcmp(tree->synT, "PP") && !strcmp(tt->synT, "NP") && tt->siblingL )
    if( strcmp(tt->siblingL->synT, "PP") && 
	strcmp(tt->siblingL->synT, "RB") && 
	strcmp(tt->siblingL->synT, "CC") && 
	strcmp(tt->siblingL->synT, "JJ") ) /*undocumented; is it ok?*/
      tt = tt->siblingL;
  
  strcpy(lex, tt->lex); strcpy(preT, tt->preT);
  tree->headchild = tt;
  return 0;
}

void lexSearchLeftPar(Tree *tree, char* list[], int nlist, char lex[], char preT[], int *found)
{
  Tree *t;
  int i;

  *found = 1;
  for(t=tree->child; t!=NULL; t=t->siblingR)
    for(i=0; i<nlist; i++)
      if( equivalentLabel(t->synT, list[i]) && t->b < t->e ){
	strcpy(lex, t->lex);
	strcpy(preT, t->preT);
	tree->headchild = t;
	return;
      }
  
  *found = 0;
  /* here when all tree below is empty */
  strcpy(lex, tree->child->lex); 
  strcpy(preT, tree->child->preT); 
  tree->headchild = tree->child;
}

void lexSearchRightPar(Tree *tree, char* list[], int nlist, char lex[], char preT[], int *found)
{
  Tree *t, *tt;
  int i;

  *found = 1;
  for(tt=tree->child; tt->siblingR!=NULL; tt=tt->siblingR)
    ;

  for(t=tt; t!=NULL; t=t->siblingL)
    for(i=0; i<nlist; i++)
      if( equivalentLabel(t->synT, list[i]) && t->b < t->e ){
	strcpy(lex, t->lex);
	strcpy(preT, t->preT);
	tree->headchild = t;
	return;
      }

  *found = 0;
  strcpy(lex, tt->lex); 
  strcpy(preT, tt->preT); 
  tree->headchild = tt;
}

int coordinatedPhrase(Tree *tree, Tree **p, Tree **l, Tree **r)
{
  Tree *t, *tt;
  int flag;
  
  flag = 0;
  for(t=tree->child; t!=NULL; t=t->siblingR){
    if( t==tree->headchild ){ flag = 1; continue; }
    if( !flag && coordinatedLabel(t, tree, l, r) && headNextRight(t,tree->headchild) && t!=tree->child )
      { *p = t; return 1; }
    if( flag && coordinatedLabel(t, tree, l, r) && t->siblingR!=NULL )
      { *p = t; return 1; }
    if( flag && !strcmp(t->synT, "PRN") && t->siblingL ){ /* undocumented */
      for(tt=t->child; tt!=NULL; tt=tt->siblingR)
	if( !strcmp(tt->synT, "CC") ) break;
      if( tt && (!strcmp(t->siblingL->synT, "VP") || !strcmp(t->siblingL->synT, "VP-C")) )
	{ *p = t; *l = tree->headchild; *r = t; return 1; }
    }
  }
  *p = 0; *l = 0; *r = 0;
  return 0;
}

int coordinatedLabel(Tree *tree, Tree *parent, Tree **l, Tree **r)
{
  Tree *t;
  char csynT1[100], csynT2[100];

  /* simplest case */
  if( tree->siblingL && tree->siblingR && !strcmp(tree->synT, "CC") ){
    *l = tree->siblingL; 
    if( !strcmp(tree->siblingL->synT, ",") )
      if( tree->siblingL->siblingL ) *l = tree->siblingL->siblingL;
    coordSynT(*l, csynT1);
    *r = tree->siblingR;
    if( !strcmp(tree->siblingR->synT, ",") )
      if( tree->siblingR->siblingR ) *r = tree->siblingR->siblingR;
    coordSynT(*r, csynT2);
    return 1; /* not neccessarly equal labels when CC */ 
  }

  /* more complicated cases */
  *l = 0; *r = 0;
  for(t=tree->siblingL; t!=NULL; t=t->siblingL)
    if( !spuriousLabel(t->synT) ) break;
  coordSynT(t, csynT1);
  if( !csynT1[0]) return 0;
  *l = t;
  
  for(t=tree->siblingR; t!=NULL; t=t->siblingR)
    if( !spuriousLabel(t->synT) ) break;
  coordSynT(t, csynT2);
  if( !csynT2[0]) return 0;
  *r = t;

  if( !strcmp(tree->synT, "CC") ) /* not neccessarly equal labels when CC */ 
    return 1;
  
  if( pseudoCoordinationLabel(tree) ){
    if( !strcmp(csynT1, csynT2) ){
      if( !strcmp(csynT1, "PP") || !strcmp(csynT1, "NP") || preTNoun(csynT1) ){
	if( !strcmp(tree->synT, "CC") ) /* coord only for CC in case of NP and PP */
	  return 1;
      }
      else
	return 1;
    }
    if( !strcmp(csynT1, "VP") && !strcmp(csynT2, "NP") )
      return 1;
  }

  *l = 0; *r = 0;
  return 0;
}

int spuriousLabel(char *synT)
{
  if( !strcmp(synT, ",") || !strcmp(synT, "RB") || 
      !strcmp(synT, "ADVP") || !strcmp(synT, "ADJP") || 
      !strcmp(synT, "CONJP") || !strcmp(synT, "PRN") )
    return 1;
  return 0;
}

int pseudoCoordinationLabel(Tree *tree)
{
  char lex[100];

  if( !strcmp(tree->synT, ":") || !strcmp(tree->synT, ",") || 
      !strcmp(tree->synT, "CONJP") || !strcmp(tree->synT, "RB") )
    return 1;
  getLex(lex, tree->lex, 0);
  if( !strcmp(tree->synT, "IN") && !strcmp(lex, "than") ) 
    return 1;
  return 0;
}

void coordSynT(Tree *tree, char synT[])
{
  if( !tree ){ synT[0] = 0; return; }

  strcpy(synT, tree->synT); 
  if( synT[strlen(synT)-1]=='C' && synT[strlen(synT)-2]=='-' ) 
    synT[strlen(synT)-2] = 0;

  if( semanticTag(tree) && strcmp(synT, "NP") ) /* observed tags: ADV, PRP, TMP */
    { synT[0] = 0; return; }
  if( synT[0]=='S' ){ strcpy(synT, "S"); return; }
  if( !strcmp(synT, "IN") || !strcmp(synT, "TO") )
    { strcpy(synT, "IN_TO"); return; }
}

int headNextRight(Tree *tree, Tree *head)
{
  if( tree->siblingR==head ) return 1;

  if( head->synT[0]=='S' && tree->siblingR )
    if( (!strcmp(tree->siblingR->synT, "ADVP") || !strcmp(tree->siblingR->synT, "RB")) )
      if( tree->siblingR->siblingR==head )
	return 1;

  return 0;
}

int addNPB(char *label)
{
  if( !strcmp(label, "NP") || !strcmp(label, "NPB") || 
      !strcmp(label, "NAC") || !strcmp(label, "NX") || 
      !strcmp(label, "ADJP") || !strcmp(label, "VP") ||
      !strcmp(label, "UCP") || !strcmp(label, "PRN") || !strcmp(label, "PP") )
    return 0;
  return 1;
}

int preTNoun(char *label)
{
  if( !strcmp(label, "NN") || !strcmp(label, "NNS") || 
      !strcmp(label, "NNP") || !strcmp(label, "NNPS") )
    return 1;
  return 0;
}

int nodeNumber(Tree *tree)
{
  Tree *t;
  int n=0;

  for(t=tree->child; t!=NULL; t=t->siblingR)
    n += nodeNumber(t);
  
  return n+1;
}

int leafNumber(Tree *tree)
{
  Tree *t;
  int n=0;

  for(t=tree->child; t!=NULL; t=t->siblingR)
    n += leafNumber(t);
  
  if( tree->child==NULL )
    return 1;

  return n;
}

int height(Tree *tree)
{
  Tree *t;
  int h, maxh;

  maxh = 0;
  for(t=tree->child; t!=NULL; t=t->siblingR)
    {
      h = height(t);
      if( maxh < h )
	maxh = h;
    }
  
  return maxh+1;
}

int width(Tree *tree)
{
  Tree *t;
  int w;

  if( !tree ) return 0;

  w = 0;
  for(t=tree->child; t!=NULL; t=t->siblingR)
      w += width(t);
  
  if( tree->child==NULL )
    if( strcmp(tree->synT, "-NONE-") )
      return 1;
    else
      return 0;

  return w;
}

int countArity(Tree *tree, int c)
{
  Tree *t;
  int r=0, k;

  for(t=tree->child, k=0; t!=NULL; t=t->siblingR, k++)
    r += countArity(t, c);

  if( c<9 && k==c )
    r += 1;
  if( c>=9 && k>=c )
    r += 1;

  return r;
}

int countTreeLeaves(Tree *tree)
{
  Tree *t;
  int cnt = 0;
  
  for(t=tree->child; t!=NULL; t=t->siblingR)
    cnt += countTreeLeaves(t);

  if( tree->child==NULL && strcmp(tree->synT, "-NONE-") )
    return 1;
  else
    return cnt;
}

int hasSYNTLeaf(Tree *tree, char *synT)
{
  Tree *t;
  
  if( tree==NULL )
    return 0;

  if( tree->child == NULL && !strcmp(tree->synT, synT) )
    return 1;

  for(t=tree->child; t!=NULL; t=t->siblingR)
    if( hasSYNTLeaf(t, synT) )
      return 1;
 
  return 0;
} 

void writeTree(Tree *tree, int indent, int tab)
{
  Tree *t;
  int i, ar;
  char lex[100];

  if( tree==NULL )
    return;

  for(i=0; i<tab; i++) printf(" ");    
  for(t=tree->child, ar=0; t!=NULL; t=t->siblingR){
    ar += 1;
  }
  if( !strcmp(tree->synT, "S1") )
    printf("(TOP "); 
  else
    printf("(%s ", tree->synT); 
  if( tree->prob )
    printf(" %.5f ", tree->prob); 

  if( ar==1 ){
    indent += strlen(tree->synT); 
    writeTree(tree->child, indent, 0);
  }
  else{
    indent += 2+strlen(tree->synT); 
    for(t=tree->child; t!=NULL; t=t->siblingR){
      if( height(tree)>2 ){
	printf("\n"); 
	writeTree(t, indent, indent);
      }
      else{
	writeTree(t, indent, 0);
      }
    }
  }

  if( !tree->child ){
    getLex(lex, tree->lex, 0);
    printf("%s) ", lex);
  }
  else
    printf(")");

  if( !strcmp(tree->synT, "S1") )
    printf("\n");
}

void writeTreeSingleLine(Tree *tree, int probFlag)
{
  Tree *t;
  char lex[100];

  if( tree==NULL )
    return;

  if( !strcmp(tree->synT, "S1") )
    printf("(TOP"); 
  else
    printf("(%s", tree->synT); 
  if( probFlag )
    printf(" %.4f", tree->prob);
    
  for(t=tree->child; t!=NULL; t=t->siblingR){
    printf(" ");
    writeTreeSingleLine(t, probFlag);
  }

  if( !tree->child ){
    getLex(lex, tree->lex, 0);
    printf(" %s)", lex);
  }
  else
    printf(")");

}

void fwriteLEXPOSLeaves(FILE *f, Tree *tree)
{
  Tree *t;

  if( tree==NULL )
    return;

  if( tree->child == NULL )
    fprintf(f, "%s %s\n", tree->lex, tree->synT);

  for(t=tree->child; t!=NULL; t=t->siblingR)
    fwriteLEXPOSLeaves(f, t);
}

void writeLEXPOSLeaves(Tree *tree)
{
  Tree *t;
  char lex[100];

  if( tree==NULL )
    return;

  if( tree->child == NULL && strcmp(tree->synT, "-NONE-") ){
    getLex(lex, tree->lex, 0);
    printf("%s %s ", lex, tree->synT);
  }
  
  for(t=tree->child; t!=NULL; t=t->siblingR)
    writeLEXPOSLeaves(t);

}

int writeLeaves(Tree *tree, int flag)
{
  Tree *t;
  char lex[100];
  int n = 0;

  if( tree) {      
      if( tree->child == NULL )
          if( strcmp(tree->synT, "-NONE-") ) /* bogus lex in Gold trees */
          {
              if( flag ){
                  getLex(lex, tree->lex, 0);
                  printf("%s ", lex);
              }
              n = 1;
          }

      for(t=tree->child; t!=NULL; t=t->siblingR)
          n += writeLeaves(t, flag);
  }
  
  return n;
}

int fwriteLeaves(FILE *f, Tree *tree, int flag)
{
  Tree *t;
  int n = 0;

  if( tree) {
      if( tree->child == NULL )
          if( strcmp(tree->synT, "-NONE-") ) /* bogus lex in Gold trees */
          {
              if( flag )
                  fprintf(f, "%s ", tree->lex);
              n = 1;
          }
      for(t=tree->child; t!=NULL; t=t->siblingR)
          n += fwriteLeaves(f, t, flag);
  }
  
  return n;
}

void fwriteNodeNewline(FILE *f, Tree *tree)
{
  Tree *t;
  char lex[1000];
  int w;

  if( tree==NULL )
    return;

  if( tree->lex ) 
    {
      getLex(lex, tree->lex, 0); w = width(tree);
      fprintf(f, "%s %s %s %d\n", tree->synT, tree->preT, lex, w);
    }
  
  for(t=tree->child; t!=NULL; t=t->siblingR)
    fwriteNodeNewline(f, t);

}

void writePOSLeaves(Tree *tree)
{
  Tree *t;

  if( tree==NULL )
    return;

  if( tree->child == NULL )
    printf("%s ", tree->synT);

  for(t=tree->child; t!=NULL; t=t->siblingR)
    writePOSLeaves(t);
}


void writePOSClusters(Tree *tree)
{
  Tree *t;
  int h;

  if( tree->child==NULL )
    return;

  h = height(tree);
  if( h>=2 && h<=3 )
    {
      writePOSLeaves(tree);
      printf(":: ");
    }
  else
    for(t=tree->child; t!=NULL; t=t->siblingR)
      writePOSClusters(t);
}

Tree* deepCopy(Tree *tree, Tree *siblingL)
{
  Tree *t;

  if( tree==NULL )
    return 0;

  if( (t = (Tree*)malloc(sizeof(Tree)))==NULL )
    { fprintf(stderr, "Cannot allocate in deepCopy()\n"); exit(4); }

  t->b = tree->b;
  t->e = tree->e;
  strcpy(t->synT, tree->synT);
  strcpy(t->lex, tree->lex);

  t->child = deepCopy(tree->child, 0);
  t->siblingR = deepCopy(tree->siblingR, t);
  t->siblingL = siblingL;

  return t;
}

void freeTree(Tree *tree)
{
  Tree *t;

  if( tree==NULL )
    return;

  for(t=tree->child; t!=NULL; t=t->siblingR)
    freeTree(t);

  allb -= sizeof(Tree);
  free(tree); 
}

int sameString(char *ts, char *ds)
{
  char tmpts[200];

  getLex(tmpts, ts, 0);

  if( !strcmp(tmpts, ds) )
    return 1;

  return 0;
}

int sameCompactString(char *ts, char *ds)
{
  char *cts, *cds;
  char buf[2000];
  int i, r=0;

  if( (cts = (char *)calloc(strlen(ts)+1, sizeof(char)))==NULL )
    { fprintf(stderr, "Cannot allocate in sameCompactString()\n"); exit(11); }
  if( (cds = (char *)calloc(strlen(ds)+1, sizeof(char)))==NULL )
    { fprintf(stderr, "Cannot allocate in sameCompactString()\n"); exit(11); }
  *cts = 0;
  *cds = 0;

  for(i=0; i<=strlen(ts); )
    {
      sscanf(ts+i, "%s", buf);
      strcat(cts, buf);
      i += strlen(buf) + 1;
    }

  for(i=0; i<=strlen(ds); )
    {
      sscanf(ds+i, "%s", buf);
      strcat(cds, buf);
      i += strlen(buf) + 1;
    }

  if( !strcmp(cts, cds) )
    r = 1;

  free(cts);
  free(cds);

  return r;
}

int almostEqual(char *s1, char *s2)
{
  double ratio = 0.80;
  char buf1[200], buf2[200];
  int i, j, hits1=0, hits2=0, total1=0, total2=0;

  for(i=0; i<strlen(s1);){
    sscanf(s1+i, "%s", buf1);
    for(j=0; j<strlen(s2);){
      sscanf(s2+j, "%s", buf2);
      if( !strcmp(buf1, buf2) ){
	hits1 += 1;
	break;
      }
      j += strlen(buf2)+1;
    }
    i += strlen(buf1)+1;
    total1 += 1;
  }

  for(j=0; j<strlen(s2);){
    sscanf(s2+j, "%s", buf2);
    for(i=0; i<strlen(s1);){
      sscanf(s1+i, "%s", buf1);
      if( !strcmp(buf1, buf2) ){
	  hits2 += 1;
	  break;
      }
      i += strlen(buf1)+1;
    }
    j += strlen(buf2)+1;
    total2 += 1;
  }

  if( hits1/(double)total1 >= ratio && hits2/(double)total2 >= ratio )
    return 1;

  return 0;
}

void getLex(char *lex, char *fullex, int lowercaseFlag)
{
  int j;

  strcpy(lex, fullex);

  for(j=strlen(lex); j>0; j--)
    if( lex[j]=='[' )
      { lex[j] = 0; break; }

  if( !strcmp(lex, "Inc .") )
    strcpy(lex, "Inc.");

  if( lowercaseFlag )
    for(j=0; j<strlen(lex); j++)
      if( lex[j]>='A' && lex[j]<='Z' )
	lex[j] = lex[j] - 'A' + 'a';  
}

int getBLex(char *lex)
{
  int i, b = -1;

  for(i=strlen(lex); i>=0; i--)
    if( lex[i]=='[' )
      {
	sscanf(lex+i+1, "%d", &b);
	break;
      }
  return b;
}

int getELex(char *lex)
{
  int i, e = -1;

  for(i=strlen(lex); i>=0; i--)
    if( lex[i]=='-' )
      {
	sscanf(lex+i+1, "%d", &e);
	break;
      }
  return e;
}

int isPunctuation(char *s)
{
  if( s[0]=='.' || s[0]==',' || s[0]=='\'' || s[0]=='`' || 
      s[0]=='?' || s[0]=='!' || s[0]==':' || s[0]=='-' )
    return 1;

  return 0;
}

int dominatesSynTNP(Tree *tree)
{
  Tree *t;

  if( !strcmp(tree->synT, "S") ) /* S's are being repaired later */
    return 0;

  if( !strcmp(tree->synT, "PRN") ) /* undocumented: see WSJ_0286.MRG/57 (Ltd) */
    return 0;

  if( !strcmp(tree->synT, "NX") ) /* undocumented: see WSJ_0319.MRG/10 (months) */
    return 0;

  if( !strcmp(tree->synT, "NP") )
    return 1;

  for(t=tree->child; t!=NULL; t=t->siblingR){
    if( dominatesSynTNP(t) )
      return 1;
  }
  return 0;
}

/* for API; string-level head determination */

int ruleHead(char *node)
{
  int h, nmod;
  char *p, parent[SLEN], mod[MAXMODS][SLEN];

  p = node;
  sscanf(p, "%s", parent);
  p += strlen(parent)+1;

  nmod = 0;
  while( sscanf(p, "%s", mod[nmod])!=EOF && p<node+strlen(node) ){
    p += strlen(mod[nmod])+1;
    nmod++;
  }
  if( nmod==0 ) return 0;

  h = getHead(parent, mod, nmod);
  if( h<=nmod-2 && !strcmp(mod[h], "CC") ){}
  else if( h>=2 && !strcmp(mod[h-2], "CC") )
    h = h-2;
  return h;
}

int getHead(char parent[], char mod[][SLEN], int nmod)
{
  int i;

  if( isLabel(parent, "ADJP") )
    return lexSearchLeftInList(mod, nmod, ADJP, nADJP);
      
  if( isLabel(parent, "ADVP") )
    return lexSearchRightInList(mod, nmod, ADVP, nADVP);

  if( isLabel(parent, "CONJP") )
    return lexSearchRightInList(mod, nmod, CONJP, nCONJP);

  if( isLabel(parent, "FRAG") )
    return lexSearchRightInList(mod, nmod, FRAG, nFRAG);

  if( isLabel(parent, "INTJ") )
    return lexSearchLeftInList(mod, nmod, INTJ, nINTJ);

  if( isLabel(parent, "LST") )
    return lexSearchRightInList(mod, nmod, LST, nLST);

  if( isLabel(parent, "NAC") )
    return lexSearchLeftInList(mod, nmod, NAC, nNAC);

  if( isLabel(parent, "PP") )
    return lexSearchRightInList(mod, nmod, PP, nPP);

  if( isLabel(parent, "PRN") )
    return lexSearchLeftInList(mod, nmod, PRN, nPRN);

  if( isLabel(parent, "PRT") )
    return lexSearchRightInList(mod, nmod, PRT, nPRT);

  if( isLabel(parent, "QP") )
    return lexSearchLeftInList(mod, nmod, QP, nQP);

  if( isLabel(parent, "RRC") )
    return lexSearchRightInList(mod, nmod, RRC, nRRC);

  if( isLabel(parent, "S") || isLabel(parent, "SG") )
    return lexSearchLeftInList(mod, nmod, S, nS);

  if( isLabel(parent, "SBAR") )
    return lexSearchLeftInList(mod, nmod, SBAR, nSBAR);

  if( isLabel(parent, "SBARQ") )
    return lexSearchLeftInList(mod, nmod, SBARQ, nSBARQ);

  if( isLabel(parent, "SINV") )
    return lexSearchLeftInList(mod, nmod, SINV, nSINV);

  if( isLabel(parent, "SQ") )
    return lexSearchLeftInList(mod, nmod, SQ, nSQ);

  if( isLabel(parent, "UCP") )
    return lexSearchRightInList(mod, nmod, UCP, nUCP);

  if( isLabel(parent, "VP") )
    return lexSearchLeftInList(mod, nmod, VP, nVP);

  if( isLabel(parent, "WHADJP") )
    return lexSearchLeftInList(mod, nmod, WHADJP, nWHADJP);

  if( isLabel(parent, "WHADVP") )
    return lexSearchRightInList(mod, nmod, WHADVP, nWHADVP);

  if( isLabel(parent, "WHNP") )
    return lexSearchLeftInList(mod, nmod, WHNP, nWHNP);

  if( isLabel(parent, "WHPP") )
    return lexSearchRightInList(mod, nmod, WHPP, nWHPP);

  if( isLabel(parent, "NP") || isLabel(parent, "NPB") )
    {
      if( !strcmp(mod[nmod-1], "POS") ) return nmod;
      if( (i=lexSearchRightParInList(mod, nmod, NP1, nNP1)) ) return i; 
      if( (i= lexSearchLeftParInList(mod, nmod, NP2, nNP2)) ) return i;
      if( (i=lexSearchRightParInList(mod, nmod, NP3, nNP3)) ) return i;
      if( (i=lexSearchRightParInList(mod, nmod, NP4, nNP4)) ) return i;
      if( (i=lexSearchRightParInList(mod, nmod, NP5, nNP5)) ) return i;
      /* here? default rule for right */
      return nmod;
    }

  /* here? default rule */
  return 1;
}

int lexSearchLeftInList(char mod[][SLEN], int nmod, char* list[], int nlist)
{
  int i, j;

  for(i=0; i<nlist; i++)
    for(j=0; j<nmod; j++)
      if( equivalentLabel(mod[j], list[i]) )
	return j+1;

  if( nlist==0 ){ /* if empty (PRN), take the left-most non-punc child, if exists; is this ok?*/
    if( (!strcmp(mod[0], ",") || !strcmp(mod[0], ":")) && nmod>0 )
      return 2;
    return 1;
  }

  return 0;
}

int lexSearchRightInList(char mod[][SLEN], int nmod, char* list[], int nlist)
{
  int i, j;

  for(i=0; i<nlist; i++)
    for(j=nmod-1; j>=0; j--)
      if( equivalentLabel(mod[j], list[i]) )
	return j+1;
  
  if( nmod>0 )
    return 1;
  return 0;
}

int lexSearchLeftParInList(char mod[][SLEN], int nmod, char* list[], int nlist)
{
  int i, j;

  for(j=0; j<nmod; j++)
    for(i=0; i<nlist; i++)
      if( equivalentLabel(mod[j], list[i]) )
	return j+1;

  if( nlist==0 ){ /* if empty (PRN), take the left-most non-punc child, if exists; is this ok?*/
    if( (!strcmp(mod[0], ",") || !strcmp(mod[0], ":")) && nmod>0 )
      return 2;
    return 1;
  }

  return 0;
}

int lexSearchRightParInList(char mod[][SLEN], int nmod, char* list[], int nlist)
{
  int i, j;

  for(j=nmod-1; j>=0; j--)
    for(i=0; i<nlist; i++)
      if( equivalentLabel(mod[j], list[i]) )
	return j+1;
  
  return 0;
}
