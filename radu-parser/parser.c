#include "parser.h"

char RulesTrain[] = "../TRAINING/";
char RulesPrefix[] = "rules";
char RulesSufix[] = "joint=2";

FILE *ft, *fp, *fg, *fwCC, *fwPP, *fn, *fb, *fgb;
int SCNT;

int main(int argc, char **argv)
{ 
  char headframefile[LLEN], modfile[LLEN], Grammar[LLEN], dbset[SLEN];
  char fprefix[LLEN], ftree[LLEN], fprob[LLEN], ftime[LLEN], fbeam[LLEN], fgbeam[LLEN];
  int c, nt, i, n, rn;
  double p;

  if( argc!=7 && argc !=4 )
    { fprintf(stderr, "Usage: parser test-file Type:PTB|MT NBEST [dynbeam gbeam dyngbeam]\n"); exit(33); }

  BEAM = 9.21;
  NBEST = atoi(argv[3]);

  if( argc==7 ){ 
    DYNBEAM = atoi(argv[4]);
    GBEAM = atoi(argv[5]);
    DYNGBEAM = atoi(argv[6]);
    sprintf(dbset, ".%d.%d.%d", DYNBEAM, GBEAM, DYNGBEAM);
  }
  else{
    DYNBEAM = 0;
    GBEAM = 0;
    DYNGBEAM = 0;
    strcpy(dbset, "");
  }

  mymalloc_init();
  mymalloc_char_init();

  sprintf(headframefile, "%s%s.%s.headframe.%s", RulesTrain, RulesPrefix, argv[2], RulesSufix);
  sprintf(modfile, "%s%s.%s.mod.%s", RulesTrain, RulesPrefix, argv[2], RulesSufix);
  sprintf(Grammar, "../GRAMMAR/%s", argv[2]);

  createHash(4000003, &hashCnt);

  THCNT = 6;
  readGrammar(Grammar, THCNT);
  
  strcpy(fprefix, argv[1]); fprefix[strlen(fprefix)-3] = 0; 
  sprintf(ftree, "%s%s.JNT%s", fprefix, argv[2], dbset);
  sprintf(ftime, "%s%s.TMD%s", fprefix, argv[2], dbset);
  sprintf(fprob, "%s%s.PRB%s", fprefix, argv[2], dbset);

  if( !DYNBEAM )
    sprintf(fbeam, "%s%s.BEAM", fprefix, argv[2]);
  else
    sprintf(fbeam, "%s%s.BEAM.sort", fprefix, argv[2]);
  if( !DYNGBEAM )
    sprintf(fgbeam, "%s%s.GBEAM", fprefix, argv[2]);
  else
    sprintf(fgbeam, "%s%s.GBEAM.sort", fprefix, argv[2]);

  if( !strcmp(argv[1], "stdin") ){
    ft = NULL; 
    fp = stdout;
    fg = NULL;
  }
  else{
    if( (ft=fopen(ftree, "w"))==NULL)
      { fprintf(stderr, "Cannot open file %s\n", ftree); exit(2); }
    if( (fp=fopen(fprob, "w"))==NULL)
      { fprintf(stderr, "Cannot open file %s\n", fprob); exit(2); }
    if( (fg=fopen(ftime, "w"))==NULL)
      { fprintf(stderr, "Cannot open file %s\n", ftime); exit(2); }
    if( !DYNBEAM ){
      if( (fb=fopen(fbeam, "w"))==NULL)
	{ fprintf(stderr, "Cannot open file %s\n", fbeam); exit(2); }
    }
    else{
      if( (fb=fopen(fbeam, "r"))==NULL)
	{ fprintf(stderr, "Cannot open file %s\n", fbeam); exit(2); }
    }
    if( !DYNGBEAM ){
      if( (fgb=fopen(fgbeam, "w"))==NULL)
	{ fprintf(stderr, "Cannot open file %s\n", fgbeam); exit(2); }
    }
    else{
      if( (fgb=fopen(fgbeam, "r"))==NULL)
	{ fprintf(stderr, "Cannot open file %s\n", fgbeam); exit(2); }
    }
  }

  if( DYNBEAM )
    trainBeam(fb);
  if( DYNGBEAM )
    trainGBeam(fgb);

  if( NBEST > 1 )
    fprintf(stderr, "Parsing for %d-best list\n", NBEST);

  SBLM = 0;
  if( SBLM )
    fprintf(stderr, "Using SBLM=1 means duplicate training: under THCNT, duplicate lex as UNK\n");
  else
    ; /* fprintf(stderr, "Using SBLM=0 means regular training, and Collins style treatment of UNK\n");   */

  fprintf(stderr, "Training from %s%s.%s with THCNT=%d\n", RulesTrain, RulesPrefix, argv[2], THCNT);
  wVOC = 0;
  trainModel2(headframefile, modfile, 0);

  fprintf(stderr, "Done training\n");

  createEffHash(1000003, &effhashProb);

  while( 1 ){
    readSentences(argv[1]);
    if( nsent>0 )
      fprintf(stderr, "Sentences to be parsed: %d\n", nsent);
    else{
      fprintf(stderr, "Parser done.\n");
      break;
    }

    SCNT = 1;
    for(i=0; i<nsent; i++){
      fprintf(stderr, "Parsing sentence %3d of length %d\n", SCNT, origsent[i].nwords);
      csent = &sent[i];
      corigsent = &origsent[i];
      initChart();
      p = parse(SCNT);
      SCNT += 1;
    }
    if( strcmp(argv[1], "stdin") ){
      fclose(fg);
      fclose(fp);
      fclose(ft);
      break;
    }
  }
  fclose(fb);
  /* fclose(fgb); */

}

double parse(int cnt)
{
  int span, start, end, maxi, i, n, k;
  double fmaxprob, tprob, bprob;

  gettimeofday(&mytime,0); parseTimeS = mytime.tv_sec + mytime.tv_usec/1000000.0;

  newsentEffHash(&effhashProb);
  ntheories = 0;
  gbeamthresh = 0;
  n = csent->nwords;

  if( n==-1 ){ /* sentence longer than MAXWORDS */
    writeFlatTree(ft);
    fprintf(fp, "0\n");
    return 0;
  }

  initSent();

  if( LR==0 ){
    for(span=2; span<=n; span++){
      for(start=0; start<=n-span; start++){
	end = start+span-1;
	complete(start, end);
      }
      if( (GBEAM || DYNGBEAM) && span<n ) 
	globalThreshold(span, n);
      if( SHOWSTEPS )
	fprintf(stderr, "Done with span %d\n", span); 
    }
  }
  else{ /* Left-to-Right traversal */
    for(end=0; end<n; end++){
      for(start=end-1; start>=0; start--)
	complete(start, end);
      if( SHOWSTEPS )
	fprintf(stderr, "Reached span %d\n", end-start+1); 
    }
  }

  fmaxprob = MININF; maxi = -1;
  for(i=sindex[0][n-1]; i<=eindex[0][n-1]; i++)
    if( edges[i].labelI==labelIndex_S1 )
      if( fmaxprob < edges[i].prob ){ 
	fmaxprob = edges[i].prob;
	maxi = i;
      }
  gettimeofday(&mytime,0); parseTimeE = mytime.tv_sec + mytime.tv_usec/1000000.0;
  
  fprintf(stderr, "Prob: %.5f\n", edges[maxi].prob);
  fprintf(stderr, "Time: %.1f sec.\n", parseTimeE-parseTimeS); 
  /*
  fprintf(stderr, "Theories: %d\n", ntheories); 
  fprintf(stderr, "Theories threshed by GBEAM: %d\n", gbeamthresh);
  */

  if( fg )
    fprintf(fg, "%d %.1f %d %d\n", n, parseTimeE-parseTimeS, ntheories, nedges);
  if( edges[maxi].prob==0 && nedges==MAXEDGES-1 )
    fprintf(stderr, "Warning: the parse failed because max # of edges %d was reached prematurely\n", MAXEDGES);

  k = 0;
  if( maxi>=0 ){
    cpos = corigpos = 0;
    writeEdgeTree(ft, &edges[maxi], &edges[maxi], 0, 0, 0, 0);
    cpos = corigpos = 0;
    if( fp != stdout )
      writeEdgeTree(fp, &edges[maxi], &edges[maxi], 0, 0, 1, 1);
    else
      writeEdgeTree(fp, &edges[maxi], &edges[maxi], 0, 0, 1, 0);
    tprob = edges[maxi].prob;
    /* writeEdges(fn, &edges[maxi], &edges[maxi], labelIndex_NPB); */
    if( !DYNBEAM )
      writeBeamInfo(fb, maxi);
    if( !DYNGBEAM )
      writeGBeamInfo(fgb, maxi, maxi, n);

    for(k=1; k<NBEST; k++){
      edges[maxi].inbeam = 3; /* already written */
      fmaxprob = MININF; maxi = -1;
      for(i=sindex[0][n-1]; i<=eindex[0][n-1]; i++)
	if( edges[i].labelI==labelIndex_S1 && edges[i].inbeam<3 )
          if( fmaxprob < edges[i].prob ){
            fmaxprob = edges[i].prob;
            maxi = i;
	  }
      if( maxi>=0 ){
        cpos = corigpos = 0;
        writeEdgeTree(ft, &edges[maxi], &edges[maxi], 0, 0, 0, 0);
        cpos = corigpos = 0;
        writeEdgeTree(fp, &edges[maxi], &edges[maxi], 0, 0, 1, 1);
        bprob = edges[maxi].prob;
	if( !DYNBEAM )
          writeBeamInfo(fb, maxi);
      }
      else
	break;
    }
  }
  else{
    if( ft ) writeFlatTree(ft);
    if( fp ) fprintf(fp, "0\n");
  }
  if( NBEST>1 && k ){
    fprintf(stderr, "TOP coverages displayed: %d\n", k);
    fprintf(stderr, "Difference between top and bottom prob: %.5f\n", tprob-bprob);
  }
  if( NBEST>1 ){
    fprintf(ft, "=====\n");
    fprintf(fp, "==%d==\n", cnt);
  }
  if( ft ) fflush(ft); 
  if( fp ) fflush(fp); 
  if( fg ) fflush(fg);
  if( fb ) fflush(fb); 
  return fmaxprob;
}

int initSent()
{
  int i, j, n, li, ntag, w;
  unsigned char fkey[SLEN];
  int *tagI, unktagI[1];
  char **tag, *unktag[1];
  Key k;

  n = csent->nwords;
  for(i=0; i<n; i++){
    initSNT();

    w = csent->wordI[i]; assert(w>0);
    if( w>0 ){
      if( !unk[w] ){
	ntag = cnwordTag[w]; tag = wordTag[w]; tagI = wordTagI[w];
      }
      else{ 
	unktag[0] = csent->tag[i]; unktagI[0] = csent->tagI[i];
	ntag = 1; tag = unktag; tagI = unktagI;
      } 
    }
    /*
    else{ 
      unktag[0] = csent->tag[i]; unktagI[0] = csent->tagI[i];
      ntag = 1; tag = unktag; tagI = unktagI;
      w = UNKINDEX;
      printf("%s %s\n", csent->word[i], csent->tag[i]);
    } 
    */
    
    for(j=0; j<ntag; j++){
      edges[nedges].type = 0;
      edges[nedges].label = tag[j];
      edges[nedges].labelI = tagI[j];
      if( edges[nedges].labelI == labelIndex_S1 ) continue; /* some bug in grammar made S1 a pre-terminal */
      edges[nedges].headlabel = tag[j]; 
      edges[nedges].headlabelI = tagI[j]; 
      edges[nedges].headword = csent->word[i];
      edges[nedges].headwordI = w;
      edges[nedges].headtag = tag[j]; 
      edges[nedges].headtagI = tagI[j]; 
      edges[nedges].stop = 1;
      edges[nedges].lc.tau = START_TAU; edges[nedges].lc.vi = 0; edges[nedges].lc.subcat = 0; 
      edges[nedges].rc.tau = START_TAU; edges[nedges].rc.vi = 0; edges[nedges].rc.subcat = 0; 
      edges[nedges].start = i;
      edges[nedges].end = i;
      edges[nedges].child1 = 0;
      edges[nedges].nchildren = 0;
      edges[nedges].brick1 = edges[nedges].brick2 = -1; 
      edges[nedges].headindex = 0;
      if( baseVerb(tag[j]) ) edges[nedges].cv = 1;
      else edges[nedges].cv = 0;

      edges[nedges].prob = edges[nedges].prob2 = edges[nedges].prior = 0;

      addEdge(i, i);
    }
    addSinglesStops(i, i);
  }
  if( GBEAM || DYNGBEAM ) globalThreshold(1, n);

}

void addSinglesStops(int s, int e)
{
  int i, k;

  ADDFLAG = 0;
  addSingles(s, e, sindex[s][e], eindex[s][e]);

  nadds = &nadds1; adds = adds1; nadds1 = 0;
  addStops(s, e, sindex[s][e], eindex[s][e]);
  
  for(k=0; k<MAXUNARYREWRITES && ADDFLAG; k++){
      ADDFLAG = 0;

      nadds = &nadds2; adds = adds2; nadds2 = 0;
      for(i=0;i<nadds1;i++)
	addSingles(s, e, adds1[i], adds1[i]);

      nadds = &nadds1; adds = adds1; nadds1 = 0;
      for(i=0;i<nadds2;i++)
	addStops(s, e, adds2[i], adds2[i]);
  }

  nadds1 = nadds2 = 0;
}

void addSingles(int s, int e, int si, int ei)
{
  Edge e3;
  int i, j, l, r, pos;
  double hprob, lfprob, rfprob;

  for(i=si; i<=ei; i++)
    if( edges[i].stop && inbeam(i, s, e) )
      for(j=0; j<nunary[edges[i].labelI]; j++){
	e3 = edges[i];
	e3.type = 1;
	e3.stop = 0;
	e3.headlabel = edges[i].label;
	e3.headlabelI = edges[i].labelI;
	e3.label = unaryL[edges[i].labelI][j];
	e3.labelI = unaryI[edges[i].labelI][j];
	e3.headindex = 0;

	e3.prior = hashedComputePriorProb(e3.labelI, e3.headtagI, e3.headwordI, csent->nwords);
	hprob = hashedComputeHeadProb(edges[i].labelI, e3.labelI, edges[i].headtagI, edges[i].headwordI);

	for(l=0; l<nlframe[e3.labelI][edges[i].labelI]; l++){
	  e3.lc.tau = START_TAU; e3.lc.vi = 0; 
	  e3.lc.subcat = lframe[e3.labelI][edges[i].labelI][l];

	  if( nlframe[e3.labelI][edges[i].labelI]==1 )
	    lfprob = 0;
	  else
	    lfprob = hashedComputeFrameProb(e3.lc.subcat, e3.labelI, edges[i].labelI, edges[i].headtagI, edges[i].headwordI, 1);

	  for(r=0; r<nrframe[e3.labelI][edges[i].labelI]; r++){
	    e3.rc.tau = START_TAU; e3.rc.vi = 0; 
	    e3.rc.subcat = rframe[e3.labelI][edges[i].labelI][r];
	    if( e3.start==0 && e3.end==0 && !strcmp(e3.label, "NP-C") && !strcmp(e3.headlabel, "NPB") && !strcmp(e3.headtag, "CC") )
	      printf("");
	    
	    if( nrframe[e3.labelI][edges[i].labelI]==1 )
	      rfprob = 0;
	    else
	      rfprob = hashedComputeFrameProb(e3.rc.subcat, e3.labelI, edges[i].labelI, edges[i].headtagI, edges[i].headwordI, 2); 
	    
	    e3.prob = hprob + lfprob + rfprob + edges[i].prob;
	    e3.prob2 = e3.prob + e3.prior;

	    edges[nedges] = e3;
	    pos = addEdge(s, e);
	    if( pos>=0 ){
	      edges[pos].nchildren = 1;
	      edges[pos].child1 = nchildren;
	      children[nchildren] = i;
	      nchildren++;
	      edges[pos].brick1 = i; edges[pos].brick2 = -1;
	    }
	  }
	}
      }
}

void addStops(int s, int e, int si, int ei)
{
  Edge *lhead, *rhead;
  int i, j, pos, ec;
  double prob, lprob1, rprob1;
  int lheadlabelI, lheadtagI, lheadwordI;
  int rheadlabelI, rheadtagI, rheadwordI;
  int ltau, rtau;
  int lvi, rvi, lsubcat, rsubcat;

  for(i=si; i<=ei; i++){
    if( !edges[i].stop && inbeam(i, s, e) && 
	edges[i].lc.subcat==0 && edges[i].rc.subcat==0 ){
      edges[nedges] = edges[i];
      edges[nedges].stop = 1;
      prob = edges[i].prob;
      
      if( edges[nedges].labelI==labelIndex_NPB ){ 
	ec = children[edges[i].child1];
	lhead = &edges[ec];
	lheadlabelI = lhead->labelI; lheadtagI = lhead->headtagI; lheadwordI = lhead->headwordI;
	lsubcat = 0; lvi = 0; ltau = START_TAU;
      
	ec = children[edges[i].child1 + edges[i].nchildren - 1];
	rhead  = &edges[ec];
	rheadlabelI = rhead->labelI; rheadtagI = rhead->headtagI; rheadwordI = rhead->headwordI;
	rsubcat = 0; rvi = 0; rtau = START_TAU;
      }
      else{
	lheadlabelI = edges[i].headlabelI; lheadtagI = edges[i].headtagI; lheadwordI = edges[i].headwordI;
	lsubcat = edges[i].lc.subcat; lvi = edges[i].lc.vi; ltau = edges[i].lc.tau;
	rheadlabelI = edges[i].headlabelI; rheadtagI = edges[i].headtagI; rheadwordI = edges[i].headwordI;
	rsubcat = edges[i].rc.subcat; rvi = edges[i].rc.vi; rtau = edges[i].rc.tau;
      }

      if( edges[i].start==0 && edges[i].end==0 && !strcmp(edges[i].label, "NP-C") && !strcmp(edges[i].headlabel, "NPB") && !strcmp(edges[i].headtag, "CC") )
	printf("");
     lprob1 = hashedComputeMod1Prob(STOPNT, STOPNT, edges[i].labelI, lheadlabelI, lheadtagI, lheadwordI, lsubcat, lvi, ltau, 0, 0, 1);
      prob += lprob1;
      
      rprob1 = hashedComputeMod1Prob(STOPNT, STOPNT, edges[i].labelI, rheadlabelI, rheadtagI, rheadwordI, rsubcat, rvi, rtau, 0, 0, 2);
      prob += rprob1;
      
      edges[nedges].prob = prob;
      edges[nedges].prob2 = prob + edges[nedges].prior;
      
      pos = addEdge(s, e);
      if( pos>=0 ){ /* edge is added to the chart, so add its children */
	edges[pos].child1 = nchildren;
	edges[pos].nchildren = edges[i].nchildren;
	for(j=0; j<edges[i].nchildren; j++)
	  children[nchildren+j] = children[edges[i].child1+j];
	nchildren += edges[pos].nchildren;
	edges[pos].brick1 = i; edges[pos].brick2 = -1;
      }
    }
  }
}


void complete(int s, int e)
{
  int i, j, k, k2;
  int mod1[MAXNMOD], nmod1, head2[MAXNHEAD], nhead2, mod2[MAXNMOD], nmod2;

  if( nedges >= MAXEDGES-1 || nchildren >= MAXCHILDREN-1 )
    return;
  
  initSNT();

  for(i=s; i<e; i++){
    if( !csent->hasPunc4Prune[i] || e==csent->nwords-1 || csent->hasPunc[e] ){
      nmod1 = nmod2 = nhead2 = 0;
      for(k=sindex[i+1][e]; k<=eindex[i+1][e]; k++)
	if( inbeam2(k, i+1, e) )
	  if( edges[k].stop==1 ){
	    mod2[nmod2++] = k;
	    if( nmod2>=MAXNMOD ){ nmod2--; }
	  }
	  else{
	    head2[nhead2++] = k;
	    if( nhead2>=MAXNHEAD ){ nhead2--; }
	  }
      for(j=sindex[s][i]; j<=eindex[s][i]; j++)
	if( inbeam2(j, s, i) )
	  if( edges[j].stop==0 )
	    join2EdgesFollow(j, s, i, e, mod2, nmod2);
	  else{
	    mod1[nmod1++] = j;
	    if( nmod1>=MAXNMOD ){ nmod1--; }
          }
      
      for(k2=0; k2<nhead2; k2++){
	k = head2[k2];
	join2EdgesPrecede(k, s, i, e, mod1, nmod1);
      }
      
      if( !strcmp(csent->tag[i+1], "CC") && i < e-1 )
	for(j=sindex[s][i]; j<=eindex[s][i]; j++)
	  if( inbeam2(j, s, i) )
	    if( edges[j].stop==0 )
	      for(k=sindex[i+2][e]; k<=eindex[i+2][e]; k++)
		if( inbeam2(k, i+2, e) )
		  if( edges[k].stop==1 )
		    join2EdgesCC(j, k, s, i, e);
    }
    else{
      nmod1 = nmod2 = nhead2 = 0;
      for(k=sindex[i+1][e]; k<=eindex[i+1][e]; k++)
	if( inbeam2(k, i+1, e) )
	  if( edges[k].stop==1 ){
	    mod2[nmod2++] = k;
	    if( nmod2>=MAXNMOD ){ nmod2--; }
          }
	  else{
	    head2[nhead2++] = k;
            if( nhead2>=MAXNHEAD ){ nhead2--; }
 	  }
      
      for(j=sindex[s][i]; j<=eindex[s][i]; j++)
	if( inbeam2(j, s, i) )
	  if( edges[j].stop==0 )
	    ; /*join2EdgesFollow(j, s, i, e, mod2, nmod2); */
	  else{
	    mod1[nmod1++] = j;
	    if( nmod1>=MAXNMOD ){ nmod1--; }
          }
      
      for(k2=0; k2<nhead2; k2++){
	k = head2[k2];
	if( edges[k].labelI == labelIndex_NPB )
	  join2EdgesPrecede(k, s, i, e, mod1, nmod1);
      }
    }
  }

  addSinglesStops(s, e);
}

int join2EdgesFollow(int hi, int s, int m, int e, int *e2s, int ne2s)
{
  Edge *e1, *e2, e3;
  int i, j, k2, headvi, cvi, vi, subcat, pos, punc, tau, pt, pw;
  int headlabelI, headtagI, headwordI;
  double prob, cprob, rprob1, rprob2, pprob1, pprob2;
 
  e1 = &edges[hi];
  for(k2=0; k2<ne2s; k2++){
    e2 = &edges[e2s[k2]];

    cprob = e1->prob + e2->prob;

    headlabelI = e1->headlabelI;
    if( e1->labelI == labelIndex_NPB )
      headlabelI = edges[children[e1->child1+e1->nchildren-1]].labelI; 

    if( (sindex[s][e]>=0 && cprob < maxprob[s][e]-BEAM) || tablef[e1->labelI][headlabelI][e2->labelI]==0 )
      continue;

    e3 = *e1;
    if( e3.labelI != labelIndex_NPB )
      e3.cv = e1->cv || e2->cv; 
    else
      e3.cv = 0; 

    e3.lc = e1->lc;
    
    headvi = 0;
    if( e2->headtag[0]=='V' && e2->headtag[1]=='B' )
      headvi = 1;
    
    cvi = 0;
    if( e3.labelI!=labelIndex_NPB ){
      cvi = e2->cv;
      for(i=e1->child1+e1->headindex+1; i<e1->child1+e1->nchildren; i++)
	cvi = cvi || edges[children[i]].cv;
    }
  
    e3.rc.tau = OTHER_TAU;
    e3.rc.vi = cvi; 
    subcat = removeSubcat(e2->label, e1->rc.subcat);
    if( subcat>=0 )
      e3.rc.subcat = subcat;
    else
      continue;

    e3.start = e1->start;
    e3.end = e2->end;

    if( e1->labelI == labelIndex_NPB ){
      headlabelI = edges[children[e1->child1+e1->nchildren-1]].labelI; 
      headtagI   = edges[children[e1->child1+e1->nchildren-1]].headtagI; 
      headwordI  = edges[children[e1->child1+e1->nchildren-1]].headwordI;
      subcat = 0; vi = 0; tau = START_TAU;
    }
    else{
      headlabelI = e1->headlabelI; headtagI = e1->headtagI; headwordI = e1->headwordI;
      subcat = e1->rc.subcat; vi = e1->rc.vi; tau = e1->rc.tau;
    }

    punc = pt = pw = 0;
    if( csent->hasPunc[m] ){
      punc = 1; pt = csent->tagIPunc[m]; pw = csent->wordIPunc[m];
    }    
    
    if( e3.start==5 && e3.end==10 && !strcmp(e3.label, "SBAR") && 
	e1->start==5 && e1->end==8 && e1->rc.subcat==1000 &&
	e2->start==9 && !strcmp(e2->label, "S-C") )
      printf("");

    rprob1 = hashedComputeMod1Prob(e2->labelI, e2->headtagI, e1->labelI, headlabelI, headtagI, headwordI, subcat, vi, tau, 0, punc, 2);
    rprob2 = hashedComputeMod2Prob(e2->headwordI, e2->labelI, e2->headtagI, e1->labelI, headlabelI, headtagI, headwordI, subcat, vi, tau, 0, punc, 2);
    
    prob = cprob + rprob1 + rprob2;
    
    if( pt ){
      pprob1 = hashedComputeCP1Prob(pt, e1->labelI, headlabelI, headtagI, headwordI, e2->labelI, e2->headtagI, e2->headwordI, PUNC);
      pprob2 = hashedComputeCP2Prob(pw, pt, e1->labelI, headlabelI, headtagI, headwordI, e2->labelI, e2->headtagI, e2->headwordI, PUNC);
      prob += pprob1 + pprob2;
    }
 
    e3.prob = prob;
    e3.prob2 = prob + e3.prior;

    edges[nedges] = e3;
    pos = addEdge(s, e);
    if( pos>=0 ){
      edges[pos].child1 = nchildren;
      edges[pos].nchildren = e1->nchildren+1;
      for(j=0; j<e1->nchildren; j++)
	children[nchildren+j] = children[e1->child1+j];
      children[nchildren+j] = e2s[k2];
      nchildren += edges[pos].nchildren;
      edges[pos].brick1 = hi; edges[pos].brick2 = e2s[k2];
    }
  }
 
}

int join2EdgesCC(int hi, int mi, int s, int m, int e)
{
  Edge *e1, *e2, e3;
  int i, j, headvi, cvi, vi, subcat, pos, cc, punc, tau, pt, pw, ct, cw;
  int headlabelI, headtagI, headwordI;
  double prob, cprob, rprob1, rprob2, pprob1, pprob2, cprob1, cprob2;
 
  e1 = &edges[hi];
  e2 = &edges[mi];

  cprob = e1->prob + e2->prob;

  headlabelI = e1->headlabelI;
  if( e1->labelI == labelIndex_NPB )
    headlabelI = edges[children[e1->child1+e1->nchildren-1]].labelI; 

  if( (sindex[s][e]>=0 && cprob < maxprob[s][e]-BEAM) || tablef[e1->labelI][headlabelI][e2->labelI]==0 ) 
    return;
  
  e3 = *e1;
  if( e3.labelI != labelIndex_NPB )
    e3.cv = e1->cv || e2->cv; 
  else
    e3.cv = 0; 
  
  e3.lc = e1->lc;
  
  headvi = 0;
  if( e2->headtag[0]=='V' && e2->headtag[1]=='B' )
    headvi = 1;
  
  cvi = 0;
  if( e3.labelI!=labelIndex_NPB ){
    cvi = e2->cv;
    for(i=e1->child1+e1->headindex+1; i<e1->child1+e1->nchildren; i++)
      cvi = cvi || edges[children[i]].cv;
  }
  
  e3.rc.tau = OTHER_TAU;
  e3.rc.vi = cvi; 
  subcat = removeSubcat(e2->label, e1->rc.subcat);
  if( subcat>=0 )
    e3.rc.subcat = subcat;
  else return;

  e3.start = e1->start;
  e3.end = e2->end;

  if( e1->labelI == labelIndex_NPB ){
    headlabelI = edges[children[e1->child1+e1->nchildren-1]].labelI; 
    headtagI  = edges[children[e1->child1+e1->nchildren-1]].headtagI; 
    headwordI  = edges[children[e1->child1+e1->nchildren-1]].headwordI;
    subcat = 0; vi = 0; tau = START_TAU;
  }
  else{
    headlabelI = e1->headlabelI; headtagI = e1->headtagI; headwordI = e1->headwordI;
    subcat = e1->rc.subcat; vi = e1->rc.vi; tau = e1->rc.tau;
  }

  cc = 1; ct = csent->tagI[m+1]; cw = csent->wordI[m+1];  
  punc = pt = pw = 0;
  if( csent->hasPunc[m] ){
    punc = 1; pt = csent->tagIPunc[m]; pw = csent->wordIPunc[m];
  }    

  rprob1 = hashedComputeMod1Prob(e2->labelI, e2->headtagI, e1->labelI, headlabelI, headtagI, headwordI, subcat, vi, tau, cc, punc, 2);
  rprob2 = hashedComputeMod2Prob(e2->headwordI, e2->labelI, e2->headtagI, e1->labelI, headlabelI, headtagI, headwordI, subcat, vi, tau, cc, punc, 2);
    
  prob = cprob + rprob1 + rprob2;
    
  if( pt ){
    pprob1 = hashedComputeCP1Prob(pt, e1->labelI, headlabelI, headtagI, headwordI, e2->labelI, e2->headtagI, e2->headwordI, PUNC);
    pprob2 = hashedComputeCP2Prob(pw, pt, e1->labelI, headlabelI, headtagI, headwordI, e2->labelI, e2->headtagI, e2->headwordI, PUNC);
    prob += pprob1 + pprob2;
  }
 
  cprob1 = hashedComputeCP1Prob(ct, e1->labelI, headlabelI, headtagI, headwordI, e2->labelI, e2->headtagI, e2->headwordI, CC);
  cprob2 = hashedComputeCP2Prob(cw, ct, e1->labelI, headlabelI, headtagI, headwordI, e2->labelI, e2->headtagI, e2->headwordI, CC);
  prob += cprob1 + cprob2;

  e3.prob = prob;
  e3.prob2 = prob + e3.prior;

  edges[nedges] = e3;
  pos = addEdge(s, e);
  if( pos>=0 ){
    edges[pos].child1 = nchildren;
    edges[pos].nchildren = e1->nchildren+2;
    for(j=0; j<e1->nchildren; j++)
      children[nchildren+j] = children[e1->child1+j];
    children[nchildren+j] = sindex[m+1][m+1];
    children[nchildren+j+1] = mi;
    nchildren += edges[pos].nchildren;
    edges[pos].brick1 = hi; edges[pos].brick2 = mi; 
  }
 
}

int join2EdgesPrecede(int hi, int s, int m, int e, int *e1s, int ne1s)
{
  Edge *e1, *e2, e3;
  int i, j, k1, headvi, cvi, vi, subcat, pos, punc, tau, pt, pw;
  int headlabelI, headtagI, headwordI;
  double prob, cprob, lprob1, lprob2, pprob1, pprob2;
 
  e2 = &edges[hi];
  for(k1=0; k1<ne1s; k1++){
    e1 = &edges[e1s[k1]];

    cprob = e1->prob + e2->prob;

    headlabelI = e2->headlabelI;
    if( e2->labelI == labelIndex_NPB )
      headlabelI = edges[children[e2->child1]].labelI; 

    if( (sindex[s][e]>=0 && cprob < maxprob[s][e]-BEAM) || tablep[e2->labelI][headlabelI][e1->labelI]==0 ) 
      continue;

    e3 = *e2;
    e3.type = 1;
    e3.headindex = e2->headindex+1;
    if( e3.labelI != labelIndex_NPB )
      e3.cv = e1->cv || e2->cv; 
    else
      e3.cv = 0; 
    
    headvi = 0;
    if( e1->headtag[0]=='V' && e1->headtag[1]=='B' )
      headvi = 1;
    cvi = 0;
    if( e3.labelI != labelIndex_NPB ){
      cvi = e1->cv;
      for(i=e2->child1; i<e2->child1+e2->headindex; i++)
	cvi = cvi || edges[children[i]].cv;
    }

    e3.lc.tau = OTHER_TAU;
    e3.lc.vi = cvi; 
    subcat = removeSubcat(e1->label, e2->lc.subcat);
    if( subcat>=0 )
      e3.lc.subcat = subcat;
    else
      continue;
    e3.rc = e2->rc;
    
    e3.start = e1->start;
    e3.end = e2->end;

    if( e2->labelI == labelIndex_NPB ){
      headlabelI = edges[children[e2->child1]].labelI; 
      headtagI   = edges[children[e2->child1]].headtagI; 
      headwordI  = edges[children[e2->child1]].headwordI;
      subcat = 0; vi = 0; tau = START_TAU;
    }
    else{
      headlabelI = e2->headlabelI; headtagI = e2->headtagI; headwordI = e2->headwordI;
      subcat = e2->lc.subcat; vi = e2->lc.vi; tau = e2->lc.tau;
    }

    punc = pt = pw = 0; 
    if( csent->hasPunc[m] ){
      punc = 1; pt = csent->tagIPunc[m]; pw = csent->wordIPunc[m];
    }
      
    lprob1 = hashedComputeMod1Prob(e1->labelI, e1->headtagI, e2->labelI, headlabelI, headtagI, headwordI, subcat, vi, tau, 0, punc, 1);
    lprob2 = hashedComputeMod2Prob(e1->headwordI, e1->labelI, e1->headtagI, e2->labelI, headlabelI, headtagI, headwordI, subcat, vi, tau, 0, punc, 1);
    
    prob = cprob + lprob1 + lprob2;

    if( pt ){ 
      pprob1 = hashedComputeCP1Prob(pt, e2->labelI, headlabelI, headtagI, headwordI, e1->labelI, e1->headtagI, e1->headwordI, PUNC);
      pprob2 = hashedComputeCP2Prob(pw, pt, e2->labelI, headlabelI, headtagI, headwordI, e1->labelI, e1->headtagI, e1->headwordI, PUNC);
      prob += pprob1 + pprob2;
    }

    e3.prob = prob;
    e3.prob2 = prob + e3.prior;
    
    edges[nedges] = e3;
    pos = addEdge(s, e);
    if( pos>=0 ){
      edges[pos].child1 = nchildren;
      edges[pos].nchildren = e2->nchildren+1;
      children[nchildren] = e1s[k1];
      for(j=0; j<e2->nchildren; j++)
	children[nchildren+j+1] = children[e2->child1+j];
      nchildren += edges[pos].nchildren;
      edges[pos].brick1 = e1s[k1]; edges[pos].brick2 = hi; 
    }
  }
}

void printEdges(int s, int e)
{
  int i, indexNT[MAXNTS], cnt;

  for(i=0; i<MAXNTS; i++) indexNT[i] = 0;

  cnt = 0;
  for(i=sindex[s][e]; i<=eindex[s][e]; i++){
    fprintf(stderr, "%d : %10s(%2d) : %d : %d %.4f %.4f %d\n", i, edges[i].label, edges[i].labelI, indexNT[edges[i].labelI], edges[i].stop, edges[i].prob, edges[i].prob2, edges[i].inbeam);
    indexNT[edges[i].labelI]++;
    cnt++;
  }
  fprintf(stderr, "Total: %d\n", cnt);
}

void printLbEdges(int s, int e, char *label)
{
  int i, indexNT, cnt;

  indexNT = 0;

  cnt = 0;
  for(i=sindex[s][e]; i<=eindex[s][e]; i++){
    if( !strcmp(label, edges[i].label) ){
      fprintf(stderr, "%8d : %10s(%2d) : %d : %d %.4f %.4f %d\n", i, edges[i].label, edges[i].labelI, indexNT, edges[i].stop, edges[i].prob, edges[i].prob2, edges[i].inbeam);
      indexNT++;
      cnt++;
    }
  }
  fprintf(stderr, "Total: %d\n", cnt);
}

void writeBeamInfo(FILE *f, int p)
{
  int i, s, e, span;
  double diff;

  if( p<0 ) return;  
  if( edges[p].type==0 ) return;
  
  writeBeamInfo(f, edges[p].brick1);
  writeBeamInfo(f, edges[p].brick2);

  if( !strcmp(edges[p].label, "S1") ) return;
  s = edges[p].start; e = edges[p].end;
  span = e-s+1;
  diff = edges[maxIndex[s][e]].prob2 - edges[p].prob2;
  if( diff<0 )
    { fprintf(stderr, "Warning: maxIndex[%d][%d] is not maximal\n", s, e); exit(28); }
  fprintf(f, "%.4f %d %s %s\n", diff, span, edges[p].label, edges[maxIndex[s][e]].label);
}

void trainBeam(FILE *f)
{
  int i, j, k, wL, mL, span;
  char winLabel[SLEN];
  float d;

  for(i=0; i<MAXNTS; i++)
    for(k=0; k<MAXWORDS; k++)
      BEAMS[i][k] = BEAM;

  /* content of f assumed sorted (increasingly) */
  while( fscanf(f, "%f %d %s", &d, &span, winLabel)!=EOF ){
    wL = getLabelIndex(winLabel, 0);
    BEAMS[wL][span] = d + d/100;
  }
  close(f);
}

void trainGBeam(FILE *f)
{
  int i, j, k, wL, mL, span, maxspan;
  char winLabel[SLEN];
  float d;

  for(i=0; i<MAXNTS; i++)
    for(k=0; k<MAXWORDS; k++)
      for(j=0; j<MAXWORDS; j++)
	GBEAMS[i][k][j] = GBEAM;

  /* content of f assumed sorted (increasingly) */
  while( fscanf(f, "%f %d %s %d", &d, &span, winLabel, &maxspan)!=EOF ){
    wL = getLabelIndex(winLabel, 0);
    GBEAMS[wL][span][maxspan] = d + d/100;
  }
  close(f);
}

void initGThresh(int n)
{
  int i;

  forward[0] = 0; bp[0] = 0;
  for(i=1; i<=n; i++){ 
    forward[i] = MININF;
    bp[i] = -1;
  }
  backward[n] = 0;
  for(i=n-1; i>=0; i--){ 
    backward[i] = MININF;
  }
  bestGThreshProb = MININF;

}

void globalThreshold(int span, int n)
{
  int s, e, i, cspan, covspan;

  computeFB(span, n, 0);
  
  for(s=0; s<n; s++){
    for(cspan=span; cspan>0; cspan--){
      e = s+cspan-1;
      if( e>=n ) continue;
      for(i=sindex[s][e]; i<=eindex[s][e]; i++){
	if( edges[i].type==0 ) continue;
	if( !ingbeam(i, s, e, n) )
	  edges[i].inbeam = 0;
      }
    }
  }
}

void computeFB(int span, int n, char cov[])
{
  int s, e, i, cspan, type1Flag;
  double left, score, right, total, cprob;

  if( cov )
    cov[0] = 0;

  initGThresh(csent->nwords);

  for(s=0; s<n; s++){
    left = forward[s];
    for(cspan=span; cspan>0; cspan--){
      e = s+cspan-1;
      if( e>=n ) continue;
      for(i=sindex[s][e], type1Flag=0; i<=eindex[s][e]; i++){
	if( edges[i].type==0 ) 
	  if( i==eindex[s][e] && type1Flag==0 ) /* go through if last edge and no type==1 edge was found */
	    cprob = MINLEAFPROB;
	  else continue;
	else
	  cprob = edges[i].prob2;
	type1Flag = 1;
	score = left + cprob;
	if( forward[e+1] < score ){
	  forward[e+1] = score;
	  bp[e+1] = s;
	  bplabelI[s] = edges[i].labelI;
	}
      }
    }
  }
    
  if( bestGThreshProb < forward[n] ){
    bestGThreshProb = forward[n];
    if( cov )
      getMaxCoverage(cov, bp, bplabelI, n);
  }

  for(s=n-1; s>=0; s--){
    for(cspan=span; cspan>0; cspan--){
      e = s+cspan-1;
      if( e>=n ) continue;
      right = backward[e+1];
      for(i=sindex[s][e], type1Flag=0; i<=eindex[s][e]; i++){
	if( edges[i].type==0 ) 
	  if( i==eindex[s][e] && type1Flag==0 ) /* go through if last edge and no type==1 edge was found */
	    cprob = MINLEAFPROB;
	  else continue;
	else
	  cprob = edges[i].prob2;	
	type1Flag = 1;
	score = cprob + right;
	if( backward[s] < score )
	  backward[s] = score;
      }
    }
    if( bestGThreshProb - (forward[s]+backward[s]) < -0.0001 )
      { fprintf(stderr, "Warning: bestGThreshProb is not maximal at span %d, point %d\n", span, s); }
  }

}

int ingbeam(int i, int s, int e, int n)
{
  double total, cGBEAM;

  if( DYNGBEAM )
    cGBEAM = GBEAMS[edges[i].labelI][e-s+1][n];
  else
    cGBEAM = GBEAM;
  
  total = forward[s] + edges[i].prob2 + backward[e+1];
  if( bestGThreshProb - cGBEAM < total )
    ;
  else{
    if( inbeam(i, s, e) && edges[i].inbeam!=0 )
      gbeamthresh++;
    return 0;
  }
  return 1;
}

void getMaxCoverage(char cov[], int bp[], int bplabelI[], int n)
{
  int s, e, m;
  char bcov[1000], *bc, buf[1000], se[10];

  
  s = n;
  sprintf(se, "%d ", s);  strcpy(bcov, se);
  while( s>0 ){ 
    s = bp[s];
    strcat(bcov, labelIndex[bplabelI[s]]); strcat(bcov, " ");
    sprintf(se, "%d ", s);  strcat(bcov, se);
  }
  bcov[strlen(bcov)-1] = 0;
  bc = bcov; cov[0] = 0;
  while( sscanf(bc, "%s", buf)!=EOF ){
    bc += strlen(buf)+1;
    strcat(buf, " "); strcat(buf, cov); strcpy(cov, buf);
  }
  cov[strlen(cov)-1] = 0;

  if( !strcmp(cov, "") ) 
    fprintf(stderr, "Warning: no maxcoverage found\n");
}

void writeGBeamInfo(FILE *f, int p, int parentp, int n)
{
  int i, s, e, span, pspan, lspan, covspan;
  double total, diff;

  if( p<0 ) return;
  if( edges[p].type==0 ) return;
  
  writeGBeamInfo(f, edges[p].brick1, p, n);
  writeGBeamInfo(f, edges[p].brick2, p, n);

  if( !strcmp(edges[p].label, "S1") ) return;

  s = edges[p].start; e = edges[p].end;
  span = e-s+1;
  pspan = edges[parentp].end-edges[parentp].start+1;
  lspan = pspan>span? pspan-1 : span;

  computeFB(lspan, n, 0);
  /* edges[p] must resist gthresholding under all spans between the span of its parent -1 and its own; 
     This is bacause once the parent has been created, the brick can be threshed out without problems */
  /* covspan = extractEdgeCoverage(maxcov, s, e, n, edgecov);   */
    
  total = forward[s] + edges[p].prob2 + backward[e+1];
  diff = bestGThreshProb - total;  
  if( diff<-0.00001 )
    { fprintf(stderr, "Warning: bestGThreshProb is not maximal at span %d\n", lspan); }
  else if( diff<0 ) diff = 0;
    
  fprintf(f, "%.4f %d %s %d\n", diff, span, edges[p].label, n); 
  /* fprintf(f, "%.4f %d %s %d %s\n", diff, span, edges[p].label, covspan, edgecov);  */

}

int extractEdgeCoverage(char maxcov[], int s, int e, int n, char edgecov[])
{
  char *mc, clabel[100], buf[100];
  int cs, ns, inflag, end, begin, covspan;

  mc = maxcov;
  edgecov[0] = 0;
  inflag = 0; begin = end = -1;
  sscanf(mc, "%s", buf); cs = atoi(buf); mc += strlen(buf)+1;
  while( sscanf(mc, "%s", buf)!=EOF ){
    if( cs<n ){
      strcpy(clabel, buf); mc += strlen(buf)+1;
      sscanf(mc, "%s", buf); ns = atoi(buf); mc += strlen(buf)+1;
      if( (s<=cs && ns<=e+1) || (cs<=s && e+1<=ns) || (s<ns && ns<e+1) || (cs<e+1 && e+1<ns) ){
	if( !inflag ){ begin = cs; inflag = 1; }
	strcat(edgecov, clabel); strcat(edgecov, "_");
      }
      else if( inflag ){
	end = cs; inflag = 0;
      }
      cs = ns;
    }
  }
  if( begin>=0 && end<0 ) end = n;
  covspan = end-begin;
  if( covspan < e-s+1 )
    { fprintf(stderr, "Warning: covspan of %d does not cover [%d-%d]\n", covspan, s, e); }    
  edgecov[strlen(edgecov)-1] = 0;
  return covspan;
}

