#include "rules.h"

Tree* rules2tree(FILE *f, Tree *parent, int *flag)
{
  char synT[80], preT[80], lex[80], buf[80];
  Tree *t, *head;

  fscanf(f, "%s", synT);
  fscanf(f, "%s", preT);
  fscanf(f, "%s", lex);

  if( !parent ){
    if( (t = (Tree *)malloc(sizeof(Tree)))==NULL )
      { fprintf(stderr, "Cannot allocate in readTree()\n"); exit(4); }
    else
      allb += sizeof(Tree);
    assert(!strcmp(synT, "S1"));
    t->parent = 0;
    strcpy(t->synT, synT);
    strcpy(t->preT, preT);
    strcpy(t->lex, lex);
    t->prob = 0;
    parent = t;
  }
  else{
    assert(!strcmp(parent->synT, synT));
    assert(!strcmp(parent->preT, preT));
    assert(!strcmp(parent->lex, lex));
  }

  fscanf(f, "%s", buf);
  assert(!strcmp(buf, "->"));
    
  fscanf(f, "%s", synT);
  if( (head = (Tree *)malloc(sizeof(Tree)))==NULL )
    { fprintf(stderr, "Cannot allocate in readTree()\n"); exit(4); }
  else
    allb += sizeof(Tree);
  head->parent = parent;
  parent->headchild = head;
  strcpy(head->synT,synT);
  strcpy(head->preT,preT);
  strcpy(head->lex,lex);
  head->prob = 0;

  while( strcmp(buf, "+l+") )
    fscanf(f, "%s", buf);
  *flag = readRuleMod(f, parent, head, head, "+l+"); 
  return parent;
}

int readRuleMod(FILE *f, Tree *parent, Tree *head, Tree *lastSibling, char *dir)
{
  char synT[80], preT[80], lex[80], buf[80], scoord[3], spunc[3];
  Tree *t, *mod, *ccmod, *pcmod;
  int flag, cflag;

  fscanf(f, "%s", buf);
  fscanf(f, "%s", synT);
  fscanf(f, "%s", preT);
  fscanf(f, "%s", lex);

  if( strcmp(parent->synT, "NPB") ){
    assert(!strcmp(buf, parent->synT));
    assert(!strcmp(synT, head->synT));
    assert(!strcmp(preT, head->preT));
    assert(!strcmp(lex, head->lex));
  }
  else{
    assert(!strcmp(buf, parent->synT));
    assert(!strcmp(synT, lastSibling->synT));
    assert(!strcmp(preT, lastSibling->preT));
    assert(!strcmp(lex, lastSibling->lex));    
  }

  while( strcmp(buf, "->") )
    fscanf(f, "%s", buf);
  
  fscanf(f, "%s", buf);
  if( strcmp(buf, "+STOP+") ){
    if( (mod = (Tree *)malloc(sizeof(Tree)))==NULL )
      { fprintf(stderr, "Cannot allocate in readTree()\n"); exit(4); }
    else
      allb += sizeof(Tree);
    strcpy(mod->synT, buf);
    fscanf(f, "%s", preT);
    strcpy(mod->preT, preT);
    fscanf(f, "%s", lex);
    strcpy(mod->lex, lex);
    mod->prob = 0;

    if( !strcmp(mod->synT, mod->preT) ) /* leaf */
      mod->child = 0;

    fscanf(f, "%s", scoord);
    if( strcmp(scoord, "0") ){
      if( (ccmod = (Tree *)malloc(sizeof(Tree)))==NULL )
	{ fprintf(stderr, "Cannot allocate in readTree()\n"); exit(4); }
      else
	allb += sizeof(Tree);
      fscanf(f, "%s", buf);
      strcpy(ccmod->synT, buf);
      strcpy(ccmod->preT, buf);
      fscanf(f, "%s", buf);
      strcpy(ccmod->lex, buf);
      ccmod->prob = 0;
    }
    else
      ccmod = 0;

    fscanf(f, "%s", spunc);
    if( strcmp(spunc, "0") ){
      if( (pcmod = (Tree *)malloc(sizeof(Tree)))==NULL )
	{ fprintf(stderr, "Cannot allocate in readTree()\n"); exit(4); }
      else
	allb += sizeof(Tree);
      fscanf(f, "%s", buf);
      strcpy(pcmod->synT, buf);
      strcpy(pcmod->preT, buf);
      fscanf(f, "%s", buf);
      strcpy(pcmod->lex, buf);
      pcmod->prob = 0;
    }
    else
      pcmod = 0;

    if( !strcmp(dir, "+l+") ){
      if( pcmod ){
	pcmod->siblingR = lastSibling;
	lastSibling->siblingL = pcmod;
	pcmod->parent = parent;
	lastSibling = pcmod;
      }
      assert(ccmod==0);
      mod->siblingR = lastSibling;
      lastSibling->siblingL = mod;
      mod->parent = parent;
      lastSibling = mod;
    }
    else{ /* +r+ */
      if( pcmod ){
	pcmod->siblingL = lastSibling;
	lastSibling->siblingR = pcmod;
	pcmod->parent = parent;
	lastSibling = pcmod;
      }
      if( ccmod ){
	ccmod->siblingL = lastSibling;
	lastSibling->siblingR = ccmod;
	ccmod->parent = parent;
	lastSibling = ccmod;
      }
      mod->siblingL = lastSibling;
      lastSibling->siblingR = mod;
      mod->parent = parent;
      lastSibling = mod;
    }
  }
  else{ /* +STOP+ */
    if( !strcmp(dir, "+l+") ){
      lastSibling->siblingL = 0;
      parent->child = lastSibling;
    }
    else
      lastSibling->siblingR = 0;
    fscanf(f, "%s", buf);
    assert(!strcmp(buf, "+STOP+"));
    fscanf(f, "%s", buf);
    assert(!strcmp(buf, "+STOP+"));
    fscanf(f, "%s", buf);
    if( strcmp(buf, "0") ){ /* some errors in the original event file */
       fscanf(f, "%s", buf);
       assert(!strcmp(buf, "S1")); /* that's how the errors look like */
       fscanf(f, "%s", buf);
    }
    fscanf(f, "%s", buf);
    assert(!strcmp(buf, "0"));
  }

  if( fscanf(f, "%s", buf)!=EOF ){
    if( strcmp(buf, "+hf+") ){
      if( !strcmp(dir, buf) )
	flag = readRuleMod(f, parent, head, lastSibling, dir);
      else{
	assert(!strcmp(buf, "+r+"));
	flag = readRuleMod(f, parent, head, head, "+r+");
      }
      return flag;
    }
    else{
      for(t=parent->child, flag=1; t!=NULL; t=t->siblingR)
	if( strcmp(t->synT, t->preT) ){
	  rules2tree(f, t, &cflag);
	  flag = flag && cflag;
	}
	else{ /* leaf */
	  t->child = t->headchild = 0;
	  if( baseVerb(t->synT) )
	    t->cv = 1;
	}
    }
    return flag;
  }
  return 0;
}

void extractPlainRules(Tree *tree, int cnt)
{
  int i, j, nframe, vi[SLEN], cvi, pvi, viStop;
  char *frame[SLEN], *frameL, *frameR, psynT[SLEN], csynT[SLEN];
  char gsynT[SLEN], gpreT[SLEN], glex[SLEN];
  char d[SLEN], *ctau, lex[SLEN], tlex[SLEN], plex[SLEN], clex[SLEN], spunc[SLEN], scoord[SLEN];
  char *cgsynT, *cgpreT, cglex[SLEN];
  Tree *t, *tt, *chead, *ghead;

  if( !tree ) return;

  if( tree->child ){
    getLex(lex, tree->lex, 0);

    nframe = 0;
    for(t=tree->headchild->siblingL; t!=NULL; t=t->siblingL)
      if( t->synT[strlen(t->synT)-1] == 'C' && t->synT[strlen(t->synT)-2] == '-' )
	frame[nframe++] = mapSubcat2S(t->synT);
    quicksortStrings(frame, 0, nframe-1, nframe);

    frameL = (char *)malloc(SLEN*sizeof(char));
    strcpy(frameL, "{");
    for(i=0; i<nframe; i++)
      { strcat(frameL, frame[i]); strcat(frameL, " "); }
    strcat(frameL, "}");
    nframe = 0;
    for(t=tree->headchild->siblingR; t!=NULL; t=t->siblingR)
      if( t->synT[strlen(t->synT)-1] == 'C' && t->synT[strlen(t->synT)-2] == '-' )
	frame[nframe++] = mapSubcat2S(t->synT);
    quicksortStrings(frame, 0, nframe-1, nframe);

    frameR = (char *)malloc(SLEN*sizeof(char));
    strcpy(frameR, "{");
    for(i=0; i<nframe; i++)
      { strcat(frameR, frame[i]); strcat(frameR, " "); }
    strcat(frameR, "}");
 
    if( tree->parent ){
      strcpy(gsynT, tree->parent->synT);
      strcpy(gpreT, tree->parent->preT);
      strcpy(glex, tree->parent->lex);
    }
    else{
      strcpy(gsynT, tree->synT);
      strcpy(gpreT, tree->preT);
      strcpy(glex, tree->lex);
    }

    if( MODEL3L )
      printf("+hf+ %s %s %s %s %s %s -> %s %s %s\n", gsynT, gpreT, glex, tree->synT, tree->preT, lex, tree->headchild->synT, frameL, frameR);
    else
      printf("+hf+ %s %s %s -> %s %s %s\n", tree->synT, tree->preT, lex, tree->headchild->synT, frameL, frameR);  

    /* left modifiers */
    for(t=tree->headchild,i=0; t!=NULL; t=t->siblingL,i++){
      if( i<=1 ) vi[i] = 0;
      else{ vi[i] = vi[i-1] || t->siblingR->cv; }
    }
    if( tree->child==tree->headchild)
      viStop = 0;
    else
      viStop = vi[i-1] || tree->child->cv;
    
    strcpy(d, "+START+");
    strcpy(spunc, "0"); 
    for(t=tree->headchild,i=0; t!=NULL; t=t->siblingL,i++){
      if( i==0 ) continue;
      if( !strcmp(t->synT, ",") || !strcmp(t->synT, ":") ){
	strcpy(psynT, t->synT); getLex(plex, t->lex, 0);
	sprintf(spunc, "1 %s %s", psynT, plex);
	continue;
      }

      if( !strcmp(tree->synT, "NPB") ){
	for(tt=t->siblingR; tt!=tree->headchild; tt=tt->siblingR)
	  if( strcmp(tt->synT, ",") && strcmp(tt->synT, ":") ) break;
	chead = tt;
	cvi = 0; ctau = START_TAU;
	if( chead==tree->headchild ){
	  cgsynT = NAS; cgpreT = NAT; strcpy(cglex, NAW);
	}
	else{
	  for(tt=chead->siblingR; tt!=tree->headchild; tt=tt->siblingR)
	    if( strcmp(tt->synT, ",") && strcmp(tt->synT, ":") ) break;
	  ghead = tt;
	  cgsynT = tt->synT; cgpreT = tt->preT; getLex(cglex, tt->lex, 0);
	}
      }
      else{
	cgsynT = gsynT; cgpreT = gpreT; strcpy(cglex, glex);
	chead = tree->headchild;
	cvi = vi[i]; ctau = d;
      }

      getLex(lex, chead->lex, 0);
      getLex(tlex, t->lex, 0);
      if( MODEL3L )
	printf("+l+ %s %s %s %s %s %s %s %s %d %s -> %s %s %s %s %s \n", cgsynT, cgpreT, cglex, tree->synT, chead->synT, chead->preT, lex, frameL, cvi, ctau, t->synT, t->preT, tlex, "0", spunc);
      else
	printf("+l+ %s %s %s %s %s %d %s -> %s %s %s %s %s \n", tree->synT, chead->synT, chead->preT, lex, frameL, cvi, ctau, t->synT, t->preT, tlex, "0", spunc);
	
      strcpy(spunc, "0");

      strcpy(d, "+OTHER+");
      frameL = removeSubcat4S(t->synT, frameL);
    }
    if( strcmp(frameL, "{}") )
      { fprintf(stderr, "Warning: Left subcat non-empty: %s\n", frameL); exit(18); }
    
    if( !strcmp(tree->synT, "NPB") ){
      chead = tree->child; 
      cvi = 0; ctau = START_TAU;
      if( chead==tree->headchild ){
	cgsynT = NAS; cgpreT = NAT; strcpy(cglex, NAW);
      }
      else{
	for(tt=chead->siblingR; tt!=tree->headchild; tt=tt->siblingR)
	  if( strcmp(tt->synT, ",") && strcmp(tt->synT, ":") ) break;
	ghead = tt;
	cgsynT = tt->synT; cgpreT = tt->preT; getLex(cglex, tt->lex, 0);
      }
    }
    else{
      cgsynT = gsynT; cgpreT = gpreT; strcpy(cglex, glex);
      chead = tree->headchild; 
      cvi = viStop; ctau = d;
    }
    getLex(lex, chead->lex, 0);
    if( MODEL3L )
      printf("+l+ %s %s %s %s %s %s %s %s %d %s -> +STOP+ +STOP+ +STOP+ 0 0 \n", cgsynT, cgpreT, cglex, tree->synT, chead->synT, chead->preT, lex, frameL, cvi, ctau);
    else
      printf("+l+ %s %s %s %s %s %d %s -> +STOP+ +STOP+ +STOP+ 0 0 \n", tree->synT, chead->synT, chead->preT, lex, frameL, cvi, ctau);
      

    /* right modifiers */
    for(t=tree->headchild,i=0; t!=NULL; t=t->siblingR,i++){
      if( i<=1 ) vi[i] = 0;
      else{ vi[i] = vi[i-1] || t->siblingL->cv; }
    }
    for(tt=tree->headchild; tt->siblingR!=NULL; tt=tt->siblingR) ; /* last child */
    if( tree->headchild==tt ) 
      viStop = 0;
    else 
      viStop = vi[i-1] || tt->cv; 

    strcpy(d, "+START+");
    strcpy(scoord, "0"); strcpy(spunc, "0"); 
    for(t=tree->headchild,i=0; t!=NULL; t=t->siblingR,i++){
      if( i==0 ) continue;
      if( !strcmp(t->synT, "CC") && strcmp(tree->synT, "NPB") ){
	strcpy(csynT, t->synT); getLex(clex, t->lex, 0);
	sprintf(scoord, "1 %s %s", csynT, clex);
	continue;
      }
      if( !strcmp(t->synT, ",") || !strcmp(t->synT, ":") ){
	strcpy(psynT, t->synT); getLex(plex, t->lex, 0);
	sprintf(spunc, "1 %s %s", psynT, plex);
	continue;
      }

      if( !strcmp(tree->synT, "NPB") ){
	for(tt=t->siblingL;tt!=tree->headchild;tt=tt->siblingL)
	  if( strcmp(tt->synT, ",") && strcmp(tt->synT, ":") ) break;
	chead = tt; 
	cvi = 0; ctau = START_TAU;
	if( chead==tree->headchild ){
	  cgsynT = NAS; cgpreT = NAT; strcpy(cglex, NAW);
	}
	else{
	  for(tt=chead->siblingL; tt!=tree->headchild; tt=tt->siblingL)
	    if( strcmp(tt->synT, ",") && strcmp(tt->synT, ":") ) break;
	  ghead = tt;
	  cgsynT = tt->synT; cgpreT = tt->preT; getLex(cglex, tt->lex, 0);
	}
      }
      else{
	cgsynT = gsynT; cgpreT = gpreT; strcpy(cglex, glex);
	chead = tree->headchild; 
	cvi = vi[i]; ctau = d;
      }

      if( !strcmp(chead->synT, "protection") || !strcmp(t->preT, "Uruguay")){
	fprintf(stderr, "Problem at tree %d\n", cnt);
      }

      getLex(lex, chead->lex, 0);
      getLex(tlex, t->lex, 0);
      if( MODEL3L )
	printf("+r+ %s %s %s %s %s %s %s %s %d %s -> %s %s %s %s %s \n", cgsynT, cgpreT, cglex, tree->synT, chead->synT, chead->preT, lex, frameR, cvi, ctau, t->synT, t->preT, tlex, scoord, spunc);
      else
	printf("+r+ %s %s %s %s %s %d %s -> %s %s %s %s %s \n", tree->synT, chead->synT, chead->preT, lex, frameR, cvi, ctau, t->synT, t->preT, tlex, scoord, spunc);

      strcpy(scoord, "0");
      strcpy(spunc, "0");
      strcpy(d, "+OTHER+");
      frameR = removeSubcat4S(t->synT, frameR);     
    }
    if( strcmp(frameR, "{}") )
      { fprintf(stderr, "Warning: Right subcat non-empty: %s\n", frameR); exit(18); }
    
    if( !strcmp(tree->synT, "NPB") ){
      for(t=tree->headchild; t->siblingR!=NULL; t=t->siblingR) ;
      chead = t;
      cvi = 0; ctau = START_TAU;
      if( chead==tree->headchild ){
	cgsynT = NAS; cgpreT = NAT; strcpy(cglex, NAW);
      }
      else{
	for(tt=chead->siblingL; tt!=tree->headchild; tt=tt->siblingL)
	  if( strcmp(tt->synT, ",") && strcmp(tt->synT, ":") ) break;
	ghead = tt;
	cgsynT = tt->synT; cgpreT = tt->preT; getLex(cglex, tt->lex, 0);
      }
    }
    else{
      chead = tree->headchild;
      cvi = viStop; ctau = d;
    }
    getLex(lex, chead->lex, 0);
    if( MODEL3L )
      printf("+r+ %s %s %s %s %s %s %s %s %d %s -> +STOP+ +STOP+ +STOP+ 0 0 \n", cgsynT, cgpreT, cglex, tree->synT, chead->synT, chead->preT, lex, frameR, cvi, ctau);
    else
      printf("+r+ %s %s %s %s %s %d %s -> +STOP+ +STOP+ +STOP+ 0 0 \n", tree->synT, chead->synT, chead->preT, lex, frameR, cvi, ctau);
  }

  for(t=tree->child; t!=NULL; t=t->siblingR)
    extractPlainRules(t, cnt);
}

char* mapSubcat2S(char *synT)
{
  if( strcmp(synT, "NP-C") && strcmp(synT, "S-C") && strcmp(synT, "SG-C") && strcmp(synT, "SBAR-C") && strcmp(synT, "VP-C"))
    return MISC_SUBCAT;
  else{
    if( !strcmp(synT, "SG-C") )
      return S_SUBCAT;
    else
      return synT;
  }
}

char* removeSubcat4S(char *label, char *subcat)
{
  char *rm, *p;
  int i;
  char rmsubcat[SLEN], prefix[SLEN];

  if( label[strlen(label)-1] == 'C' && label[strlen(label)-2] == '-' ){
    rm = label;
    if( strcmp(label, "NP-C") && strcmp(label, "S-C") && strcmp(label, "SG-C") && strcmp(label, "SBAR-C") && strcmp(label, "VP-C") )
      rm = MISC_SUBCAT;
    if( !strcmp(label, "SG-C") )
      rm = S_SUBCAT;
    strcpy(rmsubcat, subcat);
    if( p=strstr(subcat, rm) ){
      for(i=0; i<p-subcat; i++)
	prefix[i] = subcat[i];
      prefix[i] = 0;
      sprintf(rmsubcat, "%s%s", prefix, p+strlen(rm)+1);
      return strdup(rmsubcat);
    }
    else
      return subcat;
  }
  else
    return subcat;
}

void quicksortStrings(char* R[], int p, int r, int N)
{
 int q;

 if ((p<r) && (N>0))
 {
  q=partitionStrings(R,p,r);
  quicksortStrings(R,p,q,N);
  quicksortStrings(R,q+1,r,N-q+p-1);
 }
}

int partitionStrings(char* R[], int p, int r)
{
 int i, j;

 i=p-1;
 j=r+1;
 while(1)
 {
  do j--; while (lessLex(R[p],R[j]));
  do i++; while (lessLex(R[i],R[p]));
  if (i<j)
    swapStrings(&R[i], &R[j]);
  else return(j);
 }
}

void swapStrings(char **r1, char **r2)
{
  char *rtmp;

  rtmp = *r1;
  *r1 = *r2;
  *r2 = rtmp;
}
