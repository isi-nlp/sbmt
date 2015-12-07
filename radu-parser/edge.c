#include "parser.h"

int addEdge(int s, int e)
{
  int i, j, nt, maxflag;

  if( edges[nedges].prob2 < MININF )
    return -1;

  ntheories++;

  /*inbeam=2 means we haven't checked whether the edge falls within the beam
    or not yet, as not all edges have been added for span s...e*/
  edges[nedges].inbeam = 2; 

  maxflag = 0;
  if(sindex[s][e]==-1){ /*first edge to be added for this span*/
      if(edges[nedges].type==0)		
	maxprob[s][e] = MININF; /*don't count POS tags in beam comparisons*/
      else
	{ maxprob[s][e] = edges[nedges].prob2; maxIndex[s][e] = nedges; maxflag = 1; }
  }
  else if( edges[nedges].labelI == labelIndex_S1 ){
      if(s != 0 || e != csent->nwords-1 )
	return -1;
  }
  else{
    if( edges[nedges].type && !inbeam(nedges, s, e))
	return -1;

    if( edges[nedges].prob2 > maxprob[s][e] && edges[nedges].type )
      { maxprob[s][e] = edges[nedges].prob2; maxIndex[s][e] = nedges; maxflag = 1; }
  }

  nt = edges[nedges].labelI;

  /* case 1: sNT for this NT is empty */
  if(sNT[nt].edge1 == -1){ /* initialise sNT to point to this edge*/
    sNT[nt].edge1 = nedges;
    sNT[nt].nedges = 1;
    edges[nedges].next = -1; /* no other edges in list */
    
    if( sindex[s][e] == -1) /* if first edge of *any* label then initialise sindex[s][e] */
      sindex[s][e] = nedges;
    eindex[s][e] = nedges;  /* last added for span s...e */
			       
    /* an edge has been added */
    adds[*nadds] = nedges; (*nadds)++; ADDFLAG = 1;
    
    nedges++;
    if( nedges >= MAXEDGES ){
      nedges--; return -1;
      /* fprintf(stderr, "Error: # of edges %d is passed the max value of MAXEDGES\n", nedges);  */
    }
    return nedges-1; /* return a pointer to the edge */
  }

  /* case 2: sNT for this NT is not empty */
  i = sNT[nt].edge1; /* begining of list */
  /* go down the linked list, if a matching edge is found then the
     dynamic programming step kicks in: either replace the old edge if
     it has lower probability, otherwise return without adding anything     */ 
  while(i != -1){
    if( equalEdge(&edges[i], &edges[nedges]) )
      if( edges[nedges].prob > edges[i].prob ){
	j = edges[i].next;
	edges[i] = edges[nedges];
	if( maxflag )
	  maxIndex[s][e] = i; /* maxprob[s][e] already set */
	edges[i].next = j;
	
	adds[*nadds] = i; (*nadds)++; ADDFLAG=1;
	return i;
      }
      else return -1;    

    j = i;
    i = edges[i].next;
  }
  /* no matching non-terminals have been found; add the new edge at position edges[nedges] */
  edges[j].next = nedges;
  edges[nedges].next = -1;
  sNT[nt].nedges++;
  
  adds[*nadds] = nedges; (*nadds)++; ADDFLAG=1;

  eindex[s][e] = nedges;
  nedges++;
  if( nedges >= MAXEDGES ){
    nedges--; return -1;
    /* fprintf(stderr, "Error: # of edges %d is passed the max value of MAXEDGES\n", nedges);  */
  }
  return nedges-1;
}

int equalEdge(Edge *e1, Edge *e2)
{

  if( e1->type != e2->type )
    return 0;
  if( e1->stop != e2->stop )
    return 0;
  if( e1->start != e2->start )
    return 0;
  if( e1->end != e2->end )
    return 0;
  if( e1->labelI != e2->labelI )
    return 0;
  if( e1->headwordI != e2->headwordI )
    return 0;
  if( e1->headtagI != e2->headtagI )
    return 0;
  if( e1->headlabelI != e2->headlabelI )
    return 0;
  if( e1->lc.vi != e2->lc.vi )
    return 0;
  if( e1->lc.tau != e2->lc.tau )
    return 0;
  if( e1->lc.subcat != e2->lc.subcat )
    return 0;
  if( e1->rc.vi != e2->rc.vi )
    return 0;
  if( e1->rc.tau != e2->rc.tau )
    return 0;
  if( e1->rc.subcat != e2->rc.subcat )
    return 0; 
  if( e1->cv != e2->cv )
    return 0;

  if( NBEST>1 && e1->prob!=e2->prob ){ /* assumes that equal surface and equal probs means identical trees */
    return 0;
  }

  return 1;
}

int inbeam(int edge, int s, int e)
{
  if( edges[edge].type != 1 ){
    if( edges[edge].prob2 < maxprob[s][e] - BEAM ) 
      return 0;
  }
  else
    if( edges[edge].labelI == labelIndex_NP || edges[edge].labelI == labelIndex_NPC ){
      if( edges[edge].prob2 < maxprob[s][e] - BEAM - 3 ) 
	return 0;
    }
    else
      if( edges[edge].prob2 < maxprob[s][e] - BEAM ) 
        return 0;
  
  return 1;
}

int inbeam2(int edge, int s, int e)
{
  double cBEAM;

  if(edges[edge].inbeam<=1) return edges[edge].inbeam;

  if( DYNBEAM ){
    cBEAM = BEAMS[edges[edge].labelI][e-s+1];
  }
  else{
    cBEAM = BEAM;
    if( edges[edge].labelI == labelIndex_NP || edges[edge].labelI == labelIndex_NPC )
      cBEAM -= 3;
  }

  if( edges[edge].type != 1 ){
    if( edges[edge].prob2 < maxprob[s][e] - BEAM ) /* for terminal nodes, BEAM normally */ 
      { edges[edge].inbeam = 0; return 0; }
  }
  else
    if( edges[edge].prob2 < maxprob[s][e] - cBEAM ) 
      { edges[edge].inbeam = 0; return 0; }
  
  edges[edge].inbeam = 1; 
  return 1;
}

void writeEdgeTree(FILE *f, Edge *pe, Edge *e, int indent, int tab, int prob, int oneline)
{
  int i;
  char hinfo[80];

  if( !f ) return;

  if( prob )
    sprintf(hinfo, "~%d~%d", e->nchildren, e->headindex+1);
  else
    hinfo[0] = 0;

  if( !oneline )
    for(i=0; i<tab; i++) fprintf(f, " ");    
  if( e->nchildren==0 ){
    while( strcmp(csent->word[cpos], corigsent->word[corigpos]) && corigpos<corigsent->nwords ){
      fprintf(f, "(%s %s) ", corigsent->tag[corigpos], corigsent->word[corigpos]); 
      corigpos++;
    }
    if( !mttokFlag )
      fprintf(f, "(%s ", e->label);
    else
      writeMTStyle(f, e, prob, oneline);
  }
  else
    if( e->labelI == labelIndex_S1 )
      if( prob )
	fprintf(f, "(TOP%s %.5f ", hinfo, e->prob);
      else
	fprintf(f, "(TOP%s ", hinfo);
    else{
      if( pe->labelI != labelIndex_S1 ){
	while( strcmp(csent->word[cpos], corigsent->word[corigpos]) && corigpos<corigsent->nwords ){
	  fprintf(f, "(%s %s)", corigsent->tag[corigpos], corigsent->word[corigpos]); 
	  if( oneline ) fprintf(f, " ");
	  else fprintf(f, "\n");
	  corigpos++;
	  if( !oneline )
	    for(i=0; i<tab; i++) fprintf(f, " ");    
	}
      }
      if( prob )
	fprintf(f, "(%s%s %.5f ", e->label, hinfo, e->prob); 
      else
	fprintf(f, "(%s%s ", e->label, hinfo); 
    }  
  
  if( e->nchildren==1 ){
    indent += strlen(e->label); 
    if( e->labelI == labelIndex_S1 ) indent += 2;
    writeEdgeTree(f, e, &edges[children[e->child1]], indent, 0, prob, oneline);
  }
  else{
    indent += 2+strlen(e->label); 
    for(i=0; i<e->nchildren; i++){
      if( edgeHeight(e)>2 ){
	if( oneline ) fprintf(f, " "); 
	else fprintf(f, "\n"); 
	writeEdgeTree(f, e, &edges[children[e->child1+i]], indent, indent, prob, oneline);
      }
      else{
	writeEdgeTree(f, e, &edges[children[e->child1+i]], indent, 0, prob, oneline);
      }
    }
  }

  if( e->nchildren==0 ){
    if( !mttokFlag )
      /* fprintf(f, "%s) ", wordIndex[e->headwordI]);  */
      fprintf(f, "%s) ", e->headword);
    else
      ; /* already written */
    cpos++; corigpos++;
  }
  else{
    if( pe->labelI == labelIndex_S1 && e->labelI != labelIndex_S1){
      if( !oneline ) 
	fprintf(f, "\n");
      while( corigpos < corigsent->nwords ){
	if( !oneline ) 
	  for(i=0; i<4+strlen(e->label)+strlen(pe->label); i++) fprintf(f, " ");    
	fprintf(f, "(%s %s) ", corigsent->tag[corigpos], corigsent->word[corigpos]); 
	corigpos++;
      }
    }
    fprintf(f, ") ");
    if( e->labelI == labelIndex_S1 )
      fprintf(f, "\n");
  }
}

void writeMTStyle(FILE *f, Edge *e, int prob, int oneline) /* keep in sync with convertPTBLex2MTLex(Tree*) in tree.c */
{
  char lex[200], piece[MAXPIECE][80];
  int i, j, k, n, npiece, flag;

  strcpy(lex, e->headword);
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
	{ fprintf(stderr, "Error: maximum number of pieces %d exceeded for: %s\n", MAXPIECE, lex); exit(24); }
      strcpy(piece[npiece+1], "@-@");
      npiece += 2;
    }
    else
      piece[npiece][j++] = lex[i];
  }
  piece[npiece++][j] = 0;
  if( npiece==1 ){
    fprintf(f, "(%s ", e->label);
    writeMTLex(f, e->headword, e->start, e->end);
  }
  else{
    if( prob )
      fprintf(f, "(C%s~%d~%d 1 ", e->label, npiece, npiece);
    else
      fprintf(f, "(C%s ", e->label);
    for(k=0; k<npiece; k++){
      fprintf(f, "(%s ", e->label);
      writeMTLex(f, piece[k], e->start, e->end);
    }
    fprintf(f, ") ");
  }
}

void writeMTLex(FILE *f, char *olex, int start, int end) 
{
  char lex[200];
  int i, n;

  n = strlen(olex);
  for(i=0; i<n; i++)
    if( olex[i] >= 'A' && olex[i] <= 'Z' ) /* lower case */
      lex[i] = 'a' + olex[i] - 'A'; 
    else
      lex[i] = olex[i];
  lex[n] = 0;
  
  if( !strcmp(lex, "--") && start==0 && end==0 ){ /* initial -- <-> * */
    fprintf(f, "*) ");
  }
  else if( !strcmp(lex, "''") ){ /* '' <-> " (loseless because POS is preserved as '') */
    fprintf(f, "\") ");
  }
  else if( !strcmp(lex, "``") ){ /* `` <-> " (loseless because POS is preserved as ``) */
    fprintf(f, "\") ");
  }
  else if( !strcmp(lex, "-LRB-") || !strcmp(lex, "-lrb-")){ /* -LRB- <-> (  */
    fprintf(f, "() ");
  }
  else if( !strcmp(lex, "-RRB-") || !strcmp(lex, "-rrb-") ){ /* -RRB- <-> )  */
    fprintf(f, ")) ");
  }
  else if( !strcmp(lex, "-LCB-") || !strcmp(lex, "-lcb-") ){ /* -LCB- <-> {  */
    fprintf(f, "{) ");
  }
  else if( !strcmp(lex, "-RCB-") || !strcmp(lex, "-rcb-") ){ /* -RCB- <-> }  */
    fprintf(f, "}) ");
  }  
  else
    fprintf(f, "%s) ", lex);
}

int edgeHeight(Edge *e)
{
  int i, h, maxh = 0;

  for(i=0; i<e->nchildren; i++){
    h = edgeHeight(&edges[children[e->child1+i]]);
    if( maxh < h )
      maxh = h;
  }
  return maxh+1;
}

void initChart()
{
  int i, j;

  /* init index */
  for(i=0;i<MAXWORDS;i++){
    for(j=0;j<MAXWORDS;j++){
	sindex[i][j] = -1;
	eindex[i][j] = -2;
    }
  }

  initSNT();

  nadds = &nadds1;
  adds = adds1;
  nadds1 = 0;

  nedges=0;
  nchildren=0; 
}

void initSNT()
{
  int i;

  for(i=0;i<MAXNTS;i++){
    sNT[i].edge1 = -1; 
    sNT[i].nedges = 0;
  }
}

