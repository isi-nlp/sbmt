#include "model2.h"

struct rusage *usage;

int wVOC; /* 1 means restricting loading lexical events to only test vocabulary-based */
int PPS;  /* 1 means pruned-per-sentence */ 
int SBLM; /* 1 means LM-like treatment of unk */
int IRENEHACK; /* 1 means dividing by NUNK (number of unknowns) the probs for events generating the UNK token */

FILE *vocSBLM;

int Debug; /* shows all internal values for probabilities */

int smoothLevel; /* 4 means last back-off is uniform distribution
		    5 means last back-offs are prior distribution on event and uniform distribution */

int THCNT;       /* ignores the flag from grammar.lex set by ThCnt; sets unk[] according to current THCNT */

double epsilonP; /* 0.0000000000000000001 = 10^(-19) */
int NNTS, NPTS, NFRS, NLEX, NUNK, tagNUNK[MAXNTS];

int STOPNT, CC, PUNC, HEAD, FRAME, MOD1, MOD2, LBL, TW, CP1, CP2, NAV;
int UNKINDEX, FFACTOR, FFACTOR_M2, FTERM;

int START_TAU, OTHER_TAU;
int rmC[MAXNTS], unk[MAXVOC], isnt[MAXNTS];
int labelIndex_NP, labelIndex_NPB, labelIndex_PP, labelIndex_CC, labelIndex_Comma, labelIndex_Colon, labelIndex_S1;
int labelIndex_NPC, labelIndex_SC, labelIndex_SGC, labelIndex_SBARC, labelIndex_VPC, labelIndex_NN;

int unaryI[MAXNTS][MAXNTS];

/*
  char *labelIndex[MAXNTS], *wordIndex[MAXVOC];
int nlabelIndex, nwordIndex;
int nunary[MAXNTS];
char *unaryL[MAXNTS][MAXNTS];
int lframe[MAXNTS][MAXNTS][MAXSUBCATS], nlframe[MAXNTS][MAXNTS];
int rframe[MAXNTS][MAXNTS][MAXSUBCATS], nrframe[MAXNTS][MAXNTS];

char *wordTag[MAXVOC][MAXTAGS];
int nwordTag, cnwordTag[MAXVOC];
int wordTagI[MAXVOC][MAXTAGS], totalWordCnt;

*/

int tablep[MAXNTS][MAXNTS][MAXNTS], tablef[MAXNTS][MAXNTS][MAXNTS];

Hash hashCnt;      /* for nts and lex */
JmpHash jmphashCnt;/* for events */
EffHash effhashProb;/* for probs */


void initAPI(int aSBLM, int aIRENEHACK, int awVOC, int aPPS)
{
  SBLM = aSBLM;
  IRENEHACK = aIRENEHACK;
  wVOC = awVOC;
  PPS = aPPS;
}

void initSBLM(char *train, char *grammar, int THCNT)
{
  char headframefile[LLEN], modfile[LLEN], allfile[LLEN];

  mymalloc_init();
  mymalloc_char_init();
  createHash(1000003, &hashCnt);
  if( !wVOC )
    createJmpHash(37017047, &jmphashCnt); /* please check it first to be prime!! seems not enough for PTB2-MT */
  else
    createJmpHash(13013023, &jmphashCnt); /* please check it first to be prime!! */
  createEffHash(3000017, &effhashProb);

  /* special word indexes; this is what's used in getAPIFiles.pl as well */
  UNKINDEX = 65501;
  NAV = 65502;

  readGrammar(grammar, THCNT);
  labelIndex_Colon = getLabelIndex(":", 0);
  labelIndex_Comma = getLabelIndex(",", 0);

  strcpy(headframefile, train); strcat(headframefile, ".headframe.joint=2");
  strcpy(modfile, train); strcat(modfile, ".mod.joint=2");
  if( !wVOC ){
    fprintf(stderr, "Training from %s with THCNT=%d\n", train, THCNT);
    trainModel2(headframefile, modfile);
  }
  else{
    sprintf(allfile, "%s.allpruned.vocab.??", train);
    trainModel2(grammar, allfile);
  }
  /* fprintf(stderr, "Done training\n"); */

}

void trainModel2(char *file1, char *file2)
{
  int i;

  START_TAU = 1;
  OTHER_TAU = 0;

  STOPNT = 0;
  /* special non-terminal indexes */
  CC = 241;
  PUNC = 242;
  HEAD = 243;
  FRAME = 244;
  MOD1 = 245;
  MOD2 = 246;
  LBL = 247;
  TW = 248;

  for(i=0; i<MAXNTS; i++)
    rmC[i] = i;

  /* fprintf(stderr, "Training parameters...\n"); */

  labelIndex_S1 = getLabelIndex("S1", 0);
  labelIndex_NP = getLabelIndex("NP", 0);
  labelIndex_NPB = getLabelIndex("NPB", 0);
  labelIndex_PP = getLabelIndex("PP", 0);
  labelIndex_CC = getLabelIndex("CC", 0);
  labelIndex_NPC = getLabelIndex("NP-C", 0);
  labelIndex_SC = getLabelIndex("S-C", 0);
  labelIndex_SGC = getLabelIndex("SG-C", 0);
  labelIndex_SBARC = getLabelIndex("SBAR-C", 0);
  labelIndex_VPC = getLabelIndex("VP-C", 0);
  labelIndex_NN = getLabelIndex("NN", 0);

  if( !wVOC ){
    trainHeadFrameAndPriorFromFile(file1);
    trainModAndPriorFromFile(file2);    
    fprintf(stderr, "Done\n");
  }
  else{
    if( PPS ){ /* pruned per sentence */
    }
    else{
      fprintf(stderr, "Unsupported option: wVOC and not PPS!\n"); exit(13);
      trainHeadFrameModAndPriorwVoc(file2);
    }
  }
  if( !wVOC || !PPS ){
    fprintf(stderr, "Maximum jumps in jmphash: %d\n", jmphashCnt.max_jump);
    fprintf(stderr, "Total entries: %d (%.2f percent, out of total of %d)\n", jmphashCnt.total, jmphashCnt.total/(double)jmphashCnt.size*100, jmphashCnt.size);
  }
}

void readGrammar(char *grammar, int setTHCNT)
{
  FILE *f;
  char buf[LLEN];
  int cnt, flag, w, i, t, p, h, C, l;

  THCNT = setTHCNT;     /* 6 default value */

  smoothLevel = 4;
  epsilonP = pow(10, -19);
  FFACTOR = 5;  /* default value */
  FFACTOR_M2 = 5;  /* default value */
  FTERM = 5;   /* default value */
  
  wordIndex[0] = strdup("+STOP+");
  wordIndex[UNKINDEX] = strdup("+UNK+");
  unk[UNKINDEX] = 1;
  NUNK = 1;

  sprintf(buf, "%s/grammar.nts", grammar);
  if( (f=fopen(buf, "r"))==NULL )
    { fprintf(stderr, "Cannot open file %s\n", buf); exit(22); }

  labelIndex[0] = strdup("+STOP+");
  NNTS = 0; NPTS = 0;
  while( fscanf(f, "%s", buf)!=EOF ){
    fscanf(f, "%d", &cnt);
    l = getLabelIndex(buf, 1);
    NNTS++;
    if( cnt ) isnt[l] = 1;
    else{ isnt[l] = 0; NPTS++; }
  }
  fclose(f);

  sprintf(buf, "%s/grammar.lex", grammar);
  if( (f=fopen(buf, "r"))==NULL )
    { fprintf(stderr, "Cannot open file %s\n", buf); exit(22); }

  NLEX = 0;
  while( fscanf(f, "%d", &cnt)!=EOF ){
    fscanf(f, "%d", &flag);
    fscanf(f, "%s", buf);
//    if( !strcmp(buf, "takes") ) printf("");
    w = getWordIndex(buf, 1);
    NLEX++; 
    if( cnt<THCNT ){ unk[w] = 1; NUNK++; }
    else unk[w] = 0;
    fscanf(f, "%d", &cnt);
    cnwordTag[w] = cnt;
    for(i=0; i<cnt; i++){
      fscanf(f, "%s", buf);
      t = getLabelIndex(buf, 0);
      wordTag[w][i] = strdup(buf);
      wordTagI[w][i] = t;
      if( cnt<THCNT )
	tagNUNK[t]++;
    }
  }
  fclose(f);

  sprintf(buf, "%s/grammar.uns", grammar);
  if( (f=fopen(buf, "r"))==NULL )
    { fprintf(stderr, "Cannot open file %s\n", buf); exit(22); }
  while( fscanf(f, "%s", buf)!=EOF ){
    h = getLabelIndex(buf, 0);
    fscanf(f, "%d", &cnt);
    fscanf(f, "%s", buf);
    p = getLabelIndex(buf, 0);
    unaryI[h][nunary[h]] = p;
    unaryL[h][nunary[h]]= strdup(buf);
    nunary[h]++;
  }
  fclose(f);

  sprintf(buf, "%s/grammar.cls", grammar);
  if( (f=fopen(buf, "r"))==NULL )
    { fprintf(stderr, "Cannot open file %s\n", buf); exit(22); }
  while( fscanf(f, "%s", buf)!=EOF ){
    p = getLabelIndex(buf, 0);
    fscanf(f, "%s", buf);
    h = getLabelIndex(buf, 0);
    fscanf(f, "%d", &cnt);
    fscanf(f, "%d", &C);
    lframe[p][h][nlframe[p][h]] = C;
    nlframe[p][h]++;
  }
  fclose(f);

  sprintf(buf, "%s/grammar.crs", grammar);
  if( (f=fopen(buf, "r"))==NULL )
    { fprintf(stderr, "Cannot open file %s\n", buf); exit(22); }
  while( fscanf(f, "%s", buf)!=EOF ){
    p = getLabelIndex(buf, 0);
    fscanf(f, "%s", buf);
    h = getLabelIndex(buf, 0);
    fscanf(f, "%d", &cnt);
    fscanf(f, "%d", &C);
    rframe[p][h][nrframe[p][h]] = C;
    nrframe[p][h]++;
  }
  NFRS = 21; /* this is the observed number of unique frames */

  fclose(f);  
}

void trainHeadFrameAndPriorFromFile(char *fname)
{
  FILE *f;
  char tmp[SLEN];
  char buf[SLEN], lbuf[MAXC], *p;
  char count[SLEN], slC[SLEN], srC[SLEN], sH[SLEN], sP[SLEN], sw[SLEN], st[SLEN];
  int cnt, i, lC, rC, H, P, t, w;

  if( (f=fopen(fname, "r"))==NULL)
    { fprintf(stderr, "Cannot open file %s\n", fname); exit(2); }
  fprintf(stderr, "Reading/Training head&frame events...\n");

  while( fgets(lbuf, MAXC, f)!=NULL ){
    for(p = lbuf; (*p)==' '; p++);
    sscanf(p, "%s", count); p += strlen(count)+1; 
    sscanf(p, "%s", buf); p += strlen(buf)+1; /* +hf+ */
    sscanf(p, "%s", sP); p += strlen(sP)+1;
    sscanf(p, "%s", st); p += strlen(st)+1;
    sscanf(p, "%s", sw); p += strlen(sw)+1;
    sscanf(p, "%s", buf); p += strlen(buf)+1; /* -> */
    if( strcmp(buf, "->") )
    { fprintf(stderr, "Error in reading headframe file: buf is %s\n",buf); exit(23); }
    sscanf(p, "%s", sH); p += strlen(sH)+1;
    for(i=0; p[i]!='}'; i++)
      slC[i] = p[i];
    slC[i] = p[i]; slC[i+1] = 0;
    p = p+i+2;
    for(i=0; p[i]!='}'; i++)
      srC[i] = p[i];
    srC[i] = p[i]; srC[i+1] = 0;
    
    cnt = atoi(count);
    if( cnt < HF_THRESHOLD ) break; /* assumes sorted input */

    H = getLabelIndex(sH, 1);
    P = getLabelIndex(sP, 1); t = getLabelIndex(st, 1); 
    w = getWordIndex(sw, 0);
    lC = encodeFrame(slC); rC = encodeFrame(srC); 

    updateFrameList(sP, P, sH, H, lC, rC);

    /* rmC correspondence */
    if( sP[strlen(sP)-1] == 'C' && sP[strlen(sP)-2] == '-' ){
        strcpy(tmp, sP); tmp[strlen(tmp)-2] = 0;
        rmC[P] = getLabelIndex(tmp, 0);
    }
    if( sH[strlen(sH)-1] == 'C' && sH[strlen(sH)-2] == '-' ){
        strcpy(tmp, sH); tmp[strlen(tmp)-2] = 0;
        rmC[H] = getLabelIndex(tmp, 0);
    }

    /* extract node information */
    extractNodeInfo(P, cnt);
    if( H != t )
      extractNodeInfo(H, cnt);
    else{
      extractLeafInfo(st, t, w, cnt, 0);
      updatePT(t, w, cnt); /* estimating P(w|t) in the space of lexicalized preterminals */      
    }

    /* head&frame */
    updateHead(P, t, w, H, cnt);
    updateFrame(P, H, t, w, lC, 1, cnt);
    updateFrame(P, H, t, w, rC, 2, cnt);
    if( P == labelIndex_S1 )
      updateTop(P, H, t, w, cnt);
  
    /* prior */
    updatePrior(H, t, w, cnt);
  }
  fprintf(stderr, "Done\n");
  fclose(f);
}

void trainModAndPriorFromFile(char *fname)
{
  FILE *f;
  char tmp[SLEN];
  char buf[SLEN], lbuf[MAXC], sside[10], count[SLEN], *p;
  char sC[SLEN], sH[SLEN], sP[SLEN], sw[SLEN], st[SLEN], sV[SLEN], sT[SLEN], sM[SLEN], smt[SLEN], smw[SLEN];
  char scoord[SLEN], sct[SLEN], scw[SLEN], spunc[SLEN], spt[SLEN], spw[SLEN];
  int i, cnt, side, C, H, P, w, t, V, T, M, mt, mw, cc, ct, cw, punc, pt, pw;

  if( (f=fopen(fname, "r"))==NULL)
    { fprintf(stderr, "Cannot open file %s\n", fname); exit(2); }
  fprintf(stderr, "Reading/Training mod events...\n");

  while( fgets(lbuf, MAXC, f)!=NULL ){
    for(p = lbuf; (*p)==' '; p++);
    sscanf(p, "%s", count); p += strlen(count)+1; 
    sscanf(p, "%s", sside); p += strlen(sside)+1;
    sscanf(p, "%s", sP); p += strlen(sP)+1;
    sscanf(p, "%s", sH); p += strlen(sH)+1;
    sscanf(p, "%s", st); p += strlen(st)+1;
    sscanf(p, "%s", sw); p += strlen(sw)+1;
    for(i=0; p[i]!='}'; i++)
      sC[i] = p[i];
    sC[i] = p[i]; sC[i+1] = 0;
    p = p+i+2;
    sscanf(p, "%s", sV); p += strlen(sV)+1;
    sscanf(p, "%s", sT); p += strlen(sT)+1;
    sscanf(p, "%s", buf); p += strlen(buf)+1; /* -> */
    if( strcmp(buf, "->") )
    { fprintf(stderr, "Error in reading mod file: buf is %s\n",buf); exit(23); }
    sscanf(p, "%s", sM); p += strlen(sM)+1;
    sscanf(p, "%s", smt); p += strlen(smt)+1;
    sscanf(p, "%s", smw); p += strlen(smw)+1;

    if( strcmp(sM, "+STOP+") ){
      sscanf(p, "%s", scoord); p += strlen(scoord)+1;
      if( strcmp(scoord, "0") ){ sscanf(p, "%s", sct); p += strlen(sct)+1; sscanf(p, "%s", scw); p += strlen(scw)+1; }
      else{ strcpy(sct, "0"); strcpy(scw, "0"); }
      sscanf(p, "%s", spunc); p += strlen(spunc)+1;
      if( strcmp(spunc, "0") ){ sscanf(p, "%s", spt); p += strlen(spt)+1; sscanf(p, "%s", spw); p += strlen(spw)+1; }
      else{ strcpy(spt, "0"); strcpy(spw, "0"); }
    }
    else{ strcpy(scoord, "0"); strcpy(spunc, "0"); }

    cnt = atoi(count); 
    if( cnt < MOD_THRESHOLD ) break; /* assumes sorted input */
    
    side = getSide(sside);
    P = getLabelIndex(sP, 1);
    H = getLabelIndex(sH, 1); t = getLabelIndex(st, 1); 
    w = getWordIndex(sw, 0);

    C = encodeFrame(sC); V = atoi(sV); T = getTau(sT);
    M = getLabelIndex(sM, 1); mt = getLabelIndex(smt, 1); 
    mw = getWordIndex(smw, 0);

    if( side==1 )
      tablep[P][H][M] = 1;
    else
      tablef[P][H][M] = 1;

    cc = ct = cw = 0;
    if( strcmp(scoord, "0") ){
      cc = 1; ct = getLabelIndex(sct, 1); cw = getWordIndex(scw, 0);
    }
    punc = pt = pw = 0;
    if( strcmp(spunc, "0") ){
      punc = 1; pt = getLabelIndex(spt, 1); pw = getWordIndex(spw, 0);
    }

    /* rmC correspondence */
    if( sM[strlen(sM)-1] == 'C' && sM[strlen(sM)-2] == '-' ){
      strcpy(tmp, sM); tmp[strlen(tmp)-2] = 0;
      rmC[M] = getLabelIndex(tmp, 0);
    }
    
    if( SBLM==1 )
      if(  unk[w] )  w = UNKINDEX;
    if( unk[mw] ) mw = UNKINDEX;
    if( unk[cw] ) cw = UNKINDEX;
    if( unk[pw] ) pw = UNKINDEX;

      /* extract node information */
      if( M==mt ){
	extractLeafInfo(smt, mt, mw, cnt, 0);
	updatePT(mt, mw, cnt); /* estimating P(w|t) in the space of lexicalized preterminals */
      }      
      else
	extractNodeInfo(M, cnt);
    
      if( ct ){
	extractLeafInfo(sct, ct, cw, cnt, 0);
	updatePT(ct, cw, cnt);
      }
      if( pt ){
	extractLeafInfo(spt, pt, pw, cnt, 0);
	updatePT(pt, pw, cnt);
      }
      
      /* mod */
      updateMod1(P, C, V, T, H, t, w, M, mt, cc, punc, side, cnt);
      updateMod2(P, C, V, T, H, t, w, M, mt, mw, cc, punc, side, cnt);
      if( cc != 0 ){
	updateCP1(P, H, t, w, M, mt, mw, ct, cnt, CC);
	updateCP2(P, H, t, w, M, mt, mw, ct, cw, cnt, CC);
      }
      
      if( punc != 0 ){
	updateCP1(P, H, t, w, M, mt, mw, pt, cnt, PUNC);
	updateCP2(P, H, t, w, M, mt, mw, pt, pw, cnt, PUNC);
      }
    
      /* prior */
      if( M!=STOPNT )
	updatePrior(M, mt, mw, cnt);
  }
  fprintf(stderr, "Done\n");
  fclose(f);
}

void trainSBLMpps(char *train, char *rtype, int cnt)
{
  char ppsTraining[LLEN];

  sprintf(ppsTraining, "%s/events.%s.allpruned.vocab.%d", train, rtype, cnt);
  fprintf(stderr, "Training from %s\n", train);

  newsentJmpHash(&jmphashCnt); 
  newsentEffHash(&effhashProb);
  trainHeadFrameModAndPriorwVoc(ppsTraining);

  fprintf(stderr, "Cnt = %d Maximum jumps in jmphash: %d\t", cnt, jmphashCnt.max_jump);
  fprintf(stderr, "Total entries: %d (%.2f percent, out of total of %d)\n", jmphashCnt.total, jmphashCnt.total/(double)jmphashCnt.size*100, jmphashCnt.size);
}


void trainHeadFrameModAndPriorwVoc(char *fname)
{
  FILE *f;
  char buf[SLEN], dbuf[SLEN], lbuf[MAXC], *p;
  unsigned char joint[SLEN], joint2[SLEN], cond[SLEN];
  int jcnt, ccnt, ucnt, jn, cn, side, flag1, flag2, flag3, type; 

  if( (f=fopen(fname, "r"))==NULL)
    { fprintf(stderr, "Cannot open file %s\n", fname); exit(2); }
  /* fprintf(stderr, "Reading/Training voc-restricted events...\n"); */

  while( fgets(lbuf, MAXC, f)!=NULL ){
    for(p = lbuf; (*p)==' '; p++);
    sscanf(p, "%s", buf); p += strlen(buf)+1; 
    side = 0; flag3 = 0; joint2[0] = 0;
    type = buf[1];
    if( type=='h' ) /* head */
      p = readeventHead(p, buf, joint, &jn, cond, &cn, &flag1, &flag2, &flag3);
    else if( type=='t' ) /* top */
      p = readeventTop(p, buf, joint, &jn, cond, &cn, &flag1, &flag2, &flag3);
    else if( type=='f' ) /* frame */
      p = readeventFrame(p, buf, joint, joint2, &jn, cond, &cn, &flag1, &flag2, &side);
    else if( type=='m' ) /* mod */
      p = readeventMod(p, buf, joint, &jn, cond, &cn, &flag1, &flag2, &flag3, &side);
    else if( type=='p' ) /* pt  */ 
      p = readeventPT(p, buf, joint, &jn, cond, &cn, &flag1, &flag2, &flag3);
    else if( type=='c' ) /* cp */
      p = readeventCP(p, buf, joint, &jn, cond, &cn, &flag1, &flag2, &flag3);
    else if( type=='r' ) /* prior */
      p = readeventPrior(p, buf, joint, &jn, cond, &cn, &flag1, &flag2, &flag3);
    else continue;

    sscanf(p, "%s", dbuf); p += strlen(dbuf)+1; 
    jcnt = atoi(dbuf);
    sscanf(p, "%s", dbuf); p += strlen(dbuf)+1; 
    ccnt = atoi(dbuf);
    sscanf(p, "%s", dbuf); p += strlen(dbuf)+1; 
    ucnt = atoi(dbuf);

    setCntAndOutcome(type, joint, jn, jcnt, cond, cn, ccnt, ucnt, flag1, flag2, flag3, side);
  }

  fclose(f);
}

char* readeventPrior(char *p, char buf[], unsigned char joint[], int *jn, unsigned char cond[], int *cn, int *flag1, int *flag2, int *flag3)
{
  char s[SLEN];
  int arrow = 0, cnt=1, I;

  *jn = *cn = 0;
  sscanf(p, "%s", s); 
  p += strlen(s)+1;
  while( strcmp(s, "_|_") ){
    if( !strcmp(s, "->") ){ 
      arrow = 1; cnt = 0; 
      sscanf(p, "%s", s); p += strlen(s)+1;
      continue; 
    }
    if( !arrow ){
      if( cnt<=1 ){
	if( !strcmp(s, "+LBL+") ) I = LBL;
	else if( !strcmp(s, "+TW+") ) I = TW;
	else I = getLabelIndex(s, 0);
	joint[(*jn)++] = I;
	 cond[(*cn)++] = I;
      }
      else{
	I = getWordIndex(s, 0);
	*jn = writeWord(joint, I, *jn);
	*cn = writeWord( cond, I, *cn);
      }
    }
    else{
      if( cnt==0 ){
	I = getLabelIndex(s, 0);
	joint[(*jn)++] = I;
      }
      else{
	if( !strcmp(s, "+UNK+") ) I = UNKINDEX;
	else if( !strcmp(s, "+N/A+") ) I = NAV;
	else I = getWordIndex(s, 0);
	*jn = writeWord(joint, I, *jn);
      }
    }
    cnt++;
    sscanf(p, "%s", s); p += strlen(s)+1;
  }
  if( !strcmp(buf, "+rior-1+") ){ *flag1 = 71; *flag2 = 72; *flag3 = 73; }
  else if( !strcmp(buf, "+rior-2+") ){ *flag1 = 74; *flag2 = 75; *flag3 = 76; }
  else if( !strcmp(buf, "+rior-3+") ){ *flag1 = 77; *flag2 = 78; *flag3 = 79; }
  else if( !strcmp(buf, "+rior-0+") ){ *flag1 = 81; *flag2 = 82; *flag3 = 83; }
  else{ fprintf(stderr, "Error: what is this? %s\n", buf); }

  return p;
}

char* readeventPT(char *p, char buf[], unsigned char joint[], int *jn, unsigned char cond[], int *cn, int *flag1, int *flag2, int *flag3)
{
  char s[SLEN];
  int arrow = 0, cnt=1, I;

  *jn = *cn = 0;
  sscanf(p, "%s", s); p += strlen(s)+1;
  while( strcmp(s, "_|_") ){
    if( !strcmp(s, "->") ){ 
      arrow = 1; cnt = 0; 
      sscanf(p, "%s", s); p += strlen(s)+1;
      continue; 
    }
    if( !arrow ){
      if( !strcmp(s, "+LEX+") ) I = MOD2;
      else I = getLabelIndex(s, 0);
      joint[(*jn)++] = I;
       cond[(*cn)++] = I;
    }
    else{
      if( !strcmp(s, "+UNK+") ) I = UNKINDEX;
      else if( !strcmp(s, "+N/A+") ) I = NAV;
      else I = getWordIndex(s, 0);
      *jn = writeWord(joint, I, *jn);
    }
    cnt++;
    sscanf(p, "%s", s); p += strlen(s)+1;
  }
  *flag1 = 37; *flag2 = 38; *flag3 = 39;

  return p;
}

char* readeventTop(char *p, char buf[], unsigned char joint[], int *jn, unsigned char cond[], int *cn, int *flag1, int *flag2, int *flag3)
{
  char s[SLEN];
  int arrow = 0, cnt=1, I;

  *jn = *cn = 0;
  sscanf(p, "%s", s); p += strlen(s)+1;
  while( strcmp(s, "_|_") ){
    if( !strcmp(s, "->") ){ 
      arrow = 1; cnt = 0; 
      sscanf(p, "%s", s); p += strlen(s)+1;
      continue; 
    }
    if( !arrow ){
      I = getLabelIndex(s, 0);
      joint[(*jn)++] = I;
       cond[(*cn)++] = I;
    }
    else{
      if( buf[5]=='0' ){
	I = getLabelIndex(s, 0);
	joint[(*jn)++] = I;
      }
      else{
	if( !strcmp(s, "+UNK+") ) I = UNKINDEX;
	else if( !strcmp(s, "+N/A+") ) I = NAV;
	else I = getWordIndex(s, 0);
	*jn = writeWord(joint, I, *jn);
      }
    }
    cnt++;
    sscanf(p, "%s", s); p += strlen(s)+1;
  }
  if( !strcmp(buf, "+top-0+") ){ *flag1 = 61; *flag2 = 62; *flag3 = 0;}
  else{ *flag1 = 64; *flag2 = 65; *flag3 = 66;}

  return p;
}

char* readeventHead(char *p, char buf[], unsigned char joint[], int *jn, unsigned char cond[], int *cn, int *flag1, int *flag2, int *flag3)
{
  char s[SLEN];
  int arrow = 0, cnt=1, I;

  *jn = *cn = 0;
  sscanf(p, "%s", s); p += strlen(s)+1;
  while( strcmp(s, "_|_") ){
    if( !strcmp(s, "->") ){ 
      arrow = 1; cnt = 0; 
      sscanf(p, "%s", s); p += strlen(s)+1;
      continue; 
    }
    if( cnt<=2 ){
      if( !strcmp(s, "+HEAD+") ) I = HEAD;
      else I = getLabelIndex(s, 0);
      joint[(*jn)++] = I;
      if( !arrow ) cond[(*cn)++] = I;
    }
    else{
      if( !strcmp(s, "+UNK+") ) I = UNKINDEX;
      else I = getWordIndex(s, 0);
      *jn = writeWord(joint, I, *jn);
      if( !arrow ) *cn = writeWord(cond, I, *cn);
    }
    cnt++;
    sscanf(p, "%s", s); p += strlen(s)+1;
  }
  if( !strcmp(buf, "+head-1+") ){ *flag1 = 1; *flag2 = 2; *flag3 = 3; } 
  else if( !strcmp(buf, "+head-2+") ){ *flag1 = 4; *flag2 = 5; *flag3 = 6; } 
  else { *flag1 = 7; *flag2 = 8; *flag3 = 9; } 

  return p;
}

char* readeventFrame(char *p, char buf[], unsigned char joint[], unsigned char joint2[], int *jn, unsigned char cond[], int *cn, int *flag1, int *flag2, int *side)
{
  char s[SLEN], sC[SLEN];
  int i, arrow = 0, cnt=1, I, C;

  *jn = *cn = 0;
  sscanf(p, "%s", s); p += strlen(s)+1;
  while( strcmp(s, "_|_") ){
    if( !strcmp(s, "->") ){ 
      arrow = 1; cnt = 0; 
      s[0] = 0;
      continue; 
    }
    if( arrow ){
      for(i=0; p[i]!='}'; i++)
	sC[i] = p[i];
      sC[i] = p[i]; sC[i+1] = 0;
      p = p+i+2;
      C = encodeFrame(sC);
      *jn = writeFrame(joint, C, *jn);
    }
    else{
      if( cnt==1 ){
	if( !strcmp(s, "+l+") ) *side = 1;
	else *side = 2;
      }
      else if( cnt<=4 ){
	if( !strcmp(s, "+FRAME+") ) I = FRAME;
	else I = getLabelIndex(s, 0);
	joint[(*jn)++] = I;
         cond[(*cn)++] = I;
      }
      else{
	if( !strcmp(s, "+UNK+") ) I = UNKINDEX;
	else I = getWordIndex(s, 0);
	*jn = writeWord(joint, I, *jn);
        *cn = writeWord(cond, I, *cn);
      }
    }
    cnt++;
    sscanf(p, "%s", s); p += strlen(s)+1;
  }
  if( !strcmp(buf, "+frame-1+") ){ *flag1 = 11; *flag2 = 12; } 
  else if( !strcmp(buf, "+frame-2+") ){ *flag1 = 14; *flag2 = 15; } 
  else { *flag1 = 17; *flag2 = 18; } 

  return p;
}

char* readeventMod(char *p, char buf[], unsigned char joint[], int *jn, unsigned char cond[], int *cn, int *flag1, int *flag2, int *flag3, int *side)
{
  char s[SLEN], sC[SLEN];
  int i, arrow = 0, cnt=1, I, C;

  *jn = *cn = 0;
  sscanf(p, "%s", s); p += strlen(s)+1;
  while( strcmp(s, "_|_") ){
    if( !strcmp(s, "->") ){ 
      arrow = 1; cnt = 1; 
      sscanf(p, "%s", s); p += strlen(s)+1;
      continue; 
    }
    if( !arrow ){
      if( cnt==1 ){
	if( !strcmp(s, "+l+") ) *side = 1;
	else *side = 2;
      }
      else if( cnt==2 || (cnt>=6 && cnt<=7) || (cnt>=9 && cnt<=10) ){
	if( !strcmp(s, "+MOD1+") ) I = MOD1;
	else I = getLabelIndex(s, 0);
	joint[(*jn)++] = I;
         cond[(*cn)++] = I;
	 if( cnt==6 && buf[6]=='3' ) cnt = 8;
	 if( cnt==7 && buf[6]=='2' ) cnt = 8;
      }
      else if( cnt==8 ){
	if( !strcmp(s, "+UNK+") ) I = UNKINDEX;
	else I = getWordIndex(s, 0);
	*jn = writeWord(joint, I, *jn);
	*cn = writeWord(cond, I, *cn);
      }
      else if( cnt==3 ){
	for(i=0; p[i]!='}'; i++)
	  sC[i] = p[i];
	sC[i] = p[i]; sC[i+1] = 0;
	p = p+i+2;
	C = encodeFrame(sC); 
	*jn = writeFrame(joint, C, *jn);
	*cn = writeFrame( cond, C, *cn);
      }
      else if( cnt==4 || (cnt>=11 && cnt<=12)){
	joint[(*jn)++] = atoi(s);
	cond[(*cn)++] = atoi(s);
      }
      else if( cnt==5 ){
	if( !strcmp(s, "+START+") ){
	  joint[(*jn)++] = START_TAU; cond[(*cn)++] = START_TAU;
	}
	else{ joint[(*jn)++] = OTHER_TAU; cond[(*cn)++] = OTHER_TAU; }
      }
    }
    else{
      if( buf[4]=='1' ){
	if( cnt<=2 ){
	  I = getLabelIndex(s, 0);
	  joint[(*jn)++] = I;
	}
	else
	  joint[(*jn)++] = atoi(s);
      }
      else{ /* mod2 */
	if( !strcmp(s, "+UNK+") ) I = UNKINDEX;
	else if( !strcmp(s, "+N/A+") ) I = NAV;
	else I = getWordIndex(s, 0);
	*jn = writeWord(joint, I, *jn);
      }
    }

    cnt++;
    if( cnt==3 && !arrow && buf[6]!='4' ) 
      s[0] = 0;
    else 
      { sscanf(p, "%s", s); p += strlen(s)+1; }
  }

  if( !strcmp(buf, "+mod1-1+") ){ *flag1 = 21; *flag2 = 22; *flag3 = 23; } 
  else if( !strcmp(buf, "+mod1-2+") ){ *flag1 = 24; *flag2 = 25; *flag3 = 26; } 
  else if( !strcmp(buf, "+mod1-3+") ){ *flag1 = 27; *flag2 = 28; *flag3 = 29; } 
  else if( !strcmp(buf, "+mod1-4+") ){ *flag1 = 27; *flag2 = 28; *flag3 = 29; } 
  else if( !strcmp(buf, "+mod2-1+") ){ *flag1 = 31; *flag2 = 32; *flag3 = 33; } 
  else if( !strcmp(buf, "+mod2-2+") ){ *flag1 = 34; *flag2 = 35; *flag3 = 36; } 

  return p;
}

char* readeventCP(char *p, char buf[], unsigned char joint[], int *jn, unsigned char cond[], int *cn, int *flag1, int *flag2, int *flag3)
{
  char s[SLEN];
  int arrow = 0, cnt=1, I;

  *jn = *cn = 0;
  sscanf(p, "%s", s); p += strlen(s)+1;
  while( strcmp(s, "_|_") ){
    if( !strcmp(s, "->") ){ 
      arrow = 1; cnt = 0; 
      sscanf(p, "%s", s); p += strlen(s)+1;
      continue; 
    }
    if( !arrow ){
      if( (cnt>=1 && cnt<=3) || (cnt>=5 && cnt<=6) || cnt>=8 ){
	if( !strcmp(s, "+CC+") ) I = CC;
	else if( !strcmp(s, "+PUNC+") ) I = PUNC;
	else I = getLabelIndex(s, 0);
	joint[(*jn)++] = I;
	cond[(*cn)++] = I;
	if( cnt==3 && buf[5]=='2' ) cnt = 4;
	if( cnt==6 && buf[5]=='2' ) cnt = 7;
      }
      else{
	if( !strcmp(s, "+UNK+") ) I = UNKINDEX;
	else I = getWordIndex(s, 0);
	*jn = writeWord(joint, I, *jn);
	*cn = writeWord(cond, I, *cn);
      }
    }
    else{ 
      if( buf[3]=='1' ){
	I = getLabelIndex(s, 0);
	joint[(*jn)++] = I;
      }
      else{
	if( !strcmp(s, "+UNK+") ) I = UNKINDEX;
	else if( !strcmp(s, "+N/A+") ) I = NAV;
	else I = getWordIndex(s, 0);
	*jn = writeWord(joint, I, *jn);
      }
    }
    cnt++;
    sscanf(p, "%s", s); p += strlen(s)+1;
  }
  if( !strcmp(buf, "+cp1-1+") ){ *flag1 = 41; *flag2 = 42; *flag3 = 43; } 
  else if( !strcmp(buf, "+cp1-2+") ){ *flag1 = 44; *flag2 = 45; *flag3 = 46; } 
  else if( !strcmp(buf, "+cp1-3+") ){ *flag1 = 47; *flag2 = 48; *flag3 = 49; } 
  else if( !strcmp(buf, "+cp2-1+") ){ *flag1 = 51; *flag2 = 52; *flag3 = 53; } 
  else if( !strcmp(buf, "+cp2-2+") ){ *flag1 = 54; *flag2 = 55; *flag3 = 56; } 

  return p;
}


void updatePT(int L, int w, int cnt)
{
  unsigned char joint[SLEN], cond[SLEN];
  int jn1, cn1, jn2, cn2;

  if( SBLM==1 )
    if( unk[w] ) w = UNKINDEX;

  jn1 = cn1 = 0;
  joint[jn1++] = L; jn1 = writeWord(joint, w, jn1);
   cond[cn1++] = L; 
  updateCntAndOutcome(joint, jn1, cond, cn1, cnt, 37, 38, 39, 0);

  if( L==STOPNT ) return;

  jn2 = cn2 = 0;
  joint[jn2++] = MOD2; jn2 = writeWord(joint, w, jn2);
   cond[cn2++] = MOD2;
  updateCntAndOutcome(joint, jn2, cond, cn2, cnt, 37, 38, 39, 0);
}

void updatePrior(int M, int t, int w, int cnt)
{
  unsigned char joint[SLEN], cond[SLEN];
  int jn1=0, cn1=0, jn2, cn2, jn3, cn3, jn4, cn4;

  if( unk[w] ) w = UNKINDEX;

  joint[jn1++] = t; jn2 = jn1; jn1 = writeWord(joint, w, jn1); 
   cond[cn1++] = t; cn2 = cn1; cn1 = writeWord( cond, w, cn1); 

  joint[jn1++] = M; 
  updateCntAndOutcome(joint, jn1, cond, cn1, cnt, 71, 72, 73, 0);

  joint[jn2++] = M; 
  updateCntAndOutcome(joint, jn2, cond, cn2, cnt, 74, 75, 76, 0);
  
  jn3 = cn3 = 0;
  joint[jn3++] = LBL; joint[jn3++] = M;
   cond[cn3++] = LBL;
  updateCntAndOutcome(joint, jn3, cond, cn3, cnt, 77, 78, 79, 0);
  
  jn4 = cn4 = 0;
  joint[jn4++] = TW; joint[jn4++] = t; jn4 = writeWord(joint, w, jn4);
   cond[cn4++] = TW;
  updateCnt(joint, jn4, cond, cn4, cnt, 81, 82, 0);
}

void updateHead(int P, int t, int w, int H, int cnt)
{
  unsigned char cond[SLEN], joint[SLEN];
  int jn1, cn1, jn2, cn2, jn3, cn3, jn4, cn4;

  if( SBLM==1 )
    if( unk[w] ) w = UNKINDEX;

  jn1 = cn1 = 0;
  joint[jn1++] = P; jn3 = jn1; joint[jn1++] = t; jn2 = jn1; jn1 = writeWord(joint, w, jn1); 
   cond[cn1++] = P; cn3 = cn1;  cond[cn1++] = t; cn2 = cn1; cn1 = writeWord( cond, w, cn1);  

  joint[jn1++] = H; 
  updateCntAndOutcome(joint, jn1, cond, cn1, cnt, 1, 2, 3, 0);

  joint[jn2++] = H; 
  updateCntAndOutcome(joint, jn2, cond, cn2, cnt, 4, 5, 6, 0);
  
  joint[jn3++] = H; 
  updateCntAndOutcome(joint, jn3, cond, cn3, cnt, 7, 8, 9, 0);

  jn4 = 0; cn4 = 0;
  joint[jn4++] = HEAD; joint[jn4++] = H;
   cond[cn4++] = HEAD;
  updateCntAndOutcome(joint, jn4, cond, cn4, cnt, 7, 8, 9, 0);
}

void updateFrame(int P, int H, int t, int w, int C, int side, int cnt)
{
  unsigned char cond[SLEN], joint[SLEN];
  int jn1, cn1, jn2, cn2, jn3, cn3, jn4, cn4;

  if( !SBLM )
    P = rmC[P];
  if( SBLM==1 )
    if( unk[w] ) w = UNKINDEX;

  jn1 = cn1 = 0;
  joint[jn1++] = P; joint[jn1++] = H; jn3 = jn1; joint[jn1++] = t; jn2 = jn1; jn1 = writeWord(joint, w, jn1); 
   cond[cn1++] = P;  cond[cn1++] = H; cn3 = cn1;  cond[cn1++] = t; cn2 = cn1; cn1 = writeWord( cond, w, cn1); 

  jn1 = writeFrame(joint, C, jn1);
  updateCnt(joint, jn1, cond, cn1, cnt, 11, 12, side);
  
  jn2 = writeFrame(joint, C, jn2);
  updateCnt(joint, jn2, cond, cn2, cnt, 14, 15, side);
  
  jn3 = writeFrame(joint, C, jn3);
  updateCnt(joint, jn3, cond, cn3, cnt, 17, 18, side);

  jn4 = 0; cn4 = 0;
  joint[jn4++] = FRAME; joint[jn4++] = C;
   cond[cn4++] = FRAME;
  updateCnt(joint, jn4, cond, cn4, cnt, 17, 18, side);
}

void updateTop(int P, int H, int t, int w, int cnt)
{
  unsigned char cond[SLEN], joint[SLEN];
  int jn1, cn1, jn2, cn2;

  if( SBLM==1 )
    if( unk[w] ) w = UNKINDEX;

  jn1 = cn1 = 0;
  joint[jn1++] = P; 
   cond[cn1++] = P;

  joint[jn1++] = H; joint[jn1++] = t;
  updateCnt(joint, jn1, cond, cn1, cnt, 61, 62, 0);

  jn2 = cn2 = 0;
  joint[jn2++] = P; joint[jn2++] = H; joint[jn2++] = t; jn2 = writeWord(joint, w, jn2); 
   cond[cn2++] = P;  cond[cn2++] = H;  cond[cn2++] = t; 
  updateCntAndOutcome(joint, jn2, cond, cn2, cnt, 64, 65, 66, 0);
}

void updateMod1(int P, int C, int V, int T, int H, int t, int w, int M, int mt, int cc, int punc, int side, int cnt)
{
  unsigned char cond[SLEN], joint[SLEN];
  int jn1, cn1, jn2, cn2, jn3, cn3, jn4, cn4;

  if( !SBLM )
    P = rmC[P];
  if( SBLM==1 )
    if( unk[w] ) w = UNKINDEX;

  jn1 = cn1 = 0;
  joint[jn1++] = P; jn1 = writeFrame(joint, C, jn1); joint[jn1++] = V; joint[jn1++] = T; 
   cond[cn1++] = P; cn1 = writeFrame( cond, C, cn1);  cond[cn1++] = V;  cond[cn1++] = T;  
  joint[jn1++] = H; jn3 = jn1; joint[jn1++] = t; jn2 = jn1; jn1 = writeWord(joint, w, jn1); 
   cond[cn1++] = H; cn3 = cn1;  cond[cn1++] = t; cn2 = cn1; cn1 = writeWord( cond, w, cn1); 
    
  joint[jn1++] = M; joint[jn1++] = mt; joint[jn1++] = cc; joint[jn1++] = punc; 
  updateCntAndOutcome(joint, jn1, cond, cn1, cnt, 21, 22, 23, side);

  joint[jn2++] = M; joint[jn2++] = mt; joint[jn2++] = cc; joint[jn2++] = punc; 
  updateCntAndOutcome(joint, jn2, cond, cn2, cnt, 24, 25, 26, side);
  
  joint[jn3++] = M; joint[jn3++] = mt; joint[jn3++] = cc; joint[jn3++] = punc; 
  updateCntAndOutcome(joint, jn3, cond, cn3, cnt, 27, 28, 29, side);
  
  jn4 = 0; cn4 = 0;
  joint[jn4++] = MOD1; joint[jn4++] = M; joint[jn4++] = mt; joint[jn4++] = cc; joint[jn4++] = punc; 
   cond[cn4++] = MOD1;
  updateCntAndOutcome(joint, jn4, cond, cn4, cnt, 27, 28, 29, side);
}

void updateMod2(int P, int C, int V, int T, int H, int t, int w, int M, int mt, int mw, int cc, int punc, int side, int cnt)
{
  unsigned char cond[SLEN], joint[SLEN];
  int jn1, cn1, jn2, cn2;

  if( !SBLM )
    P = rmC[P];
  if( SBLM==1 )
    if( unk[w] ) w = UNKINDEX;

  if( unk[mw] ) mw = UNKINDEX;

  jn1 = cn1 = 0;  
  joint[jn1++] = P; jn1 = writeFrame(joint, C, jn1); joint[jn1++] = V; joint[jn1++] = T; 
   cond[cn1++] = P; cn1 = writeFrame( cond, C, cn1);  cond[cn1++] = V;  cond[cn1++] = T;  
  joint[jn1++] = H; joint[jn1++] = t; jn2 = jn1; jn1 = writeWord(joint, w, jn1); 
   cond[cn1++] = H;  cond[cn1++] = t; cn2 = cn1; cn1 = writeWord( cond, w, cn1); 
  joint[jn1++] = M; joint[jn1++] = mt; joint[jn1++] = cc; joint[jn1++] = punc; 
   cond[cn1++] = M;  cond[cn1++] = mt;  cond[cn1++] = cc;  cond[cn1++] = punc;

  jn1 = writeWord(joint, mw, jn1); 
  updateCntAndOutcome(joint, jn1, cond, cn1, cnt, 31, 32, 33, side);

  joint[jn2++] = M; joint[jn2++] = mt; joint[jn2++] = cc; joint[jn2++] = punc; 
   cond[cn2++] = M;  cond[cn2++] = mt;  cond[cn2++] = cc;  cond[cn2++] = punc; 

  jn2 = writeWord(joint, mw, jn2); 
  updateCntAndOutcome(joint, jn2, cond, cn2, cnt, 34, 35, 36, side);
    
   /* estimating P(w|t) in the space of lexicalized preterminals was done in updatePT() */
}

void updateCP1(int P, int H, int t, int w, int M, int mt, int mw, int ct, int cnt, int type)
{
  unsigned char cond[SLEN], joint[SLEN];
  int jn1, cn1, jn2, cn2, jn3, cn3;

  if( !SBLM )
    P = rmC[P];
  if( SBLM==1 ){
    if( unk[w] ) w = UNKINDEX;
    if( unk[mw] ) mw = UNKINDEX;
  }

  jn1 = cn1 = 0;
  joint[jn1++] = P; joint[jn1++] = H; joint[jn1++] =  t; jn1 = writeWord(joint,  w, jn1); 
                    joint[jn1++] = M; joint[jn1++] = mt; jn1 = writeWord(joint, mw, jn1); 
		    joint[jn1++] = type;
   cond[cn1++] = P;  cond[cn1++] = H;  cond[cn1++] =  t; cn1 = writeWord( cond,  w, cn1); 
                     cond[cn1++] = M;  cond[cn1++] = mt; cn1 = writeWord( cond, mw, cn1); 
		     cond[cn1++] = type;

  joint[jn1++] = ct;		     
  updateCntAndOutcome(joint, jn1, cond, cn1, cnt, 41, 42, 43, 0);
 
  jn2 = cn2 = 0;
  joint[jn2++] = P; joint[jn2++] = H; joint[jn2++] = t; joint[jn2++] = M; joint[jn2++] = mt; 
  joint[jn2++] = type;
   cond[cn2++] = P;  cond[cn2++] = H;  cond[cn2++] = t;  cond[cn2++] = M;  cond[cn2++] = mt;      
   cond[cn2++] = type;

  joint[jn2++] = ct;
  updateCntAndOutcome(joint, jn2, cond, cn2, cnt, 44, 45, 46, 0);
 
  jn3 = cn3 = 0;
  joint[jn3++] = type; joint[jn3++] = ct;
   cond[cn3++] = type;
  updateCntAndOutcome(joint, jn3, cond, cn3, cnt, 47, 48, 49, 0);
}

void updateCP2(int P, int H, int t, int w, int M, int mt, int mw, int ct, int cw, int cnt, int type)
{
  unsigned char cond[SLEN], joint[SLEN];
  int jn1, cn1, jn2, cn2;

  if( !SBLM )
   P = rmC[P];
  if( SBLM==1 ){
    if( unk[w] ) w = UNKINDEX;
    if( unk[mw] ) mw = UNKINDEX;
  }

  if( unk[cw] ) cw = UNKINDEX;

  jn1 = cn1 = 0;
  joint[jn1++] = P; joint[jn1++] = H; joint[jn1++] =  t; jn1 = writeWord(joint,  w, jn1); 
                    joint[jn1++] = M; joint[jn1++] = mt; jn1 = writeWord(joint, mw, jn1); joint[jn1++] = ct;
		    joint[jn1++] = type;
   cond[cn1++] = P;  cond[cn1++] = H;  cond[cn1++] =  t; cn1 = writeWord( cond,  w, cn1); 
                     cond[cn1++] = M;  cond[cn1++] = mt; cn1 = writeWord( cond, mw, cn1);  cond[cn1++] = ct;
		     cond[cn1++] = type;

  jn1 = writeWord(joint, cw, jn1); 
  updateCntAndOutcome(joint, jn1, cond, cn1, cnt, 51, 52, 53, 0);
 
  jn2 = cn2 = 0;
  joint[jn2++] = P; joint[jn2++] = H; joint[jn2++] = t; joint[jn2++] = M; joint[jn2++] = mt; joint[jn2++] = ct;
  joint[jn2++] = type;
   cond[cn2++] = P;  cond[cn2++] = H;  cond[cn2++] = t;  cond[cn2++] = M;  cond[cn2++] = mt;  cond[cn2++] = ct;
   cond[cn2++] = type;

  jn2 = writeWord(joint, cw, jn2); 
  updateCntAndOutcome(joint, jn2, cond, cn2, cnt, 54, 55, 56, 0);
}

double probSBLMprior(char *node)
{
  int L, t, w;
  char buf[SLEN];
  double prob;

  if( !node ) return 0;

  sscanf(node, "%s", buf); 
  readNode(buf, "parent", &L, &t, &w, 0);
  prob = computePriorProb(L, t, w);
  return prob;
}

double probSBLM(char *node, int stopFlag)
{
  char buf[SLEN];
  double prob=0, hprob, fLprob, fRprob, mprob, m1prob, m2prob, cprob, c1prob, c2prob;
  int i, j, vi[MAXMODS], cvi, viStop, P, t, w, M[MAXMODS], mt[MAXMODS], mw[MAXMODS], frameL=0, frameR=0;
  int cc, ct, cw, punc, pt, pw, d, ctau, nL=0, nR=0, chead, cv;

  if( !node ) return 0;

  sscanf(node, "%s", buf); /* parent info */
  readNode(buf, "parent", &P, &t, &w, 0);
  node += strlen(buf)+1;

  sscanf(node, "%s", buf); /* -> */
  node += strlen(buf)+1;

  sscanf(node, "%s", buf); /* head info */
  readNode(buf, "head", &(M[0]), &(mt[0]), &(mw[0]), 0);
  node += strlen(buf)+1;

  if( t!=mt[0] || w!=mw[0] ){
    fprintf(stderr, "Error: parent and head nodes must have the same headtag/headword\n"); exit(30);
  }

  hprob = hashedComputeHeadProb(M[0], P, t, w);
  prob += hprob;
  
  sscanf(node, "%s", buf); /* : */
  node += strlen(buf)+1;

  /* left modifiers */
  sscanf(node, "%s", buf); 
  node += strlen(buf)+1;
  nL = 1; vi[0] = 0; cv = 0;
  while( 1 ){
    vi[nL] = vi[nL-1] || cv;
    if( !readNode(buf, "modL", &(M[nL]), &(mt[nL]), &(mw[nL]), &cv) ) break;
    frameL = addSubcatI(M[nL], frameL);
    nL++;
    if( nL >= MAXMODS ){
    fprintf(stderr, "Error: maximum number %d of modifiers exceeded\n", MAXMODS); exit(31);
    }
    sscanf(node, "%s", buf); 
    node += strlen(buf)+1;
  }
  viStop = vi[nL-1] || cv;
  nL--;

  if( nlframe[P][M[0]]!=1 ){
    fLprob = hashedComputeFrameProb(frameL, P, M[0], t, w, 1); 
    prob += fLprob;
  }

  d = START_TAU;

  cc = ct = cw = 0; 
  punc = pt = pw = 0;
  for(i=1; i<=nL; i++){
    if( M[i]==labelIndex_Comma || M[i]==labelIndex_Colon ){
      punc = 1;
      pt = mt[i];
      pw = mw[i];
      continue;
    }
    if( P==labelIndex_NPB ){
      for(j=i-1; j>0; j--)
	if( M[j]!=labelIndex_Comma && M[j]!=labelIndex_Colon ) break;
      chead = j;
      cvi = 0; ctau = START_TAU;
    }
    else{
      chead = 0;
      cvi = vi[i]; ctau = d;
    }
	  
    m1prob = hashedComputeMod1Prob(M[i], mt[i], P, M[chead], mt[chead], mw[chead], frameL, cvi, ctau, 0, punc, 1);
    m2prob = hashedComputeMod2Prob(mw[i], M[i], mt[i], P, M[chead], mt[chead], mw[chead], frameL, cvi, ctau, 0, punc, 1);
    prob += m1prob + m2prob;

    if( punc ){
      c1prob = hashedComputeCP1Prob(pt, P, M[chead], mt[chead], mw[chead], M[i], mt[i], mw[i], PUNC);
      c2prob = hashedComputeCP2Prob(pw, pt, P, M[chead], mt[chead], mw[chead], M[i], mt[i], mw[i], PUNC);
      cprob = c1prob + c2prob;
      prob += cprob;
      punc = pt = pw = 0;
    }

    d = OTHER_TAU;
    frameL = removeSubcatI(M[i], frameL);
  }
  if( frameL!=0 )
    { fprintf(stderr, "Warning: Left subcat non-empty: %d\n", frameL); exit(18); }
    
  if( P==labelIndex_NPB ){
    chead = nL;
    cvi = 0; ctau = START_TAU;
  }
  else{
    chead = 0;
    cvi = viStop; ctau = d;
  }
  if( stopFlag==1 || stopFlag==3 ){
    mprob = hashedComputeMod1Prob(0, 0, P, M[chead], mt[chead], mw[chead], frameL, cvi, ctau, 0, 0, 1);
    prob += mprob;
  }

  /* right modifiers */
  sscanf(node, "%s", buf); 
  node += strlen(buf)+1;
  nR = 1; vi[0] = 0; cv = 0;
  while( 1 ){
    vi[nR] = vi[nR-1] || cv;
    if( !readNode(buf, "modR", &(M[nR]), &(mt[nR]), &(mw[nR]), &cv) ) break;
    frameR = addSubcatI(M[nR], frameR);
    nR++;
    if( nR >= MAXMODS ){
      fprintf(stderr, "Error: maximum number %d of modifiers exceeded\n", MAXMODS); exit(31);
    }
    sscanf(node, "%s", buf); 
    node += strlen(buf)+1;
  }
  viStop = vi[nR-1] || cv;
  nR--;
  
  if( nrframe[P][M[0]]!=1 ){
    fRprob = hashedComputeFrameProb(frameR, P, M[0], t, w, 2); 
    prob += fRprob;
  }

  d = START_TAU;

  cc = ct = cw = 0; 
  punc = pt = pw = 0;
  for(i=1; i<=nR; i++){
    if( M[i]==labelIndex_CC && M[i]!=labelIndex_NPB ){
      cc = 1;
      ct = mt[i];
      cw = mw[i];
      continue;
    }
    if( M[i]==labelIndex_Comma || M[i]==labelIndex_Colon ){
      punc = 1;
      pt = mt[i];
      pw = mw[i];
      continue;
    }
    if( P==labelIndex_NPB ){
      for(j=i-1; j>0; j--)
	if( M[j]!=labelIndex_Comma && M[j]!=labelIndex_Colon ) break;
      chead = j;
      cvi = 0; ctau = START_TAU;
    }
    else{
      chead = 0;
      cvi = vi[i]; ctau = d;
    }
	  
    m1prob = hashedComputeMod1Prob(M[i], mt[i], P, M[chead], mt[chead], mw[chead], frameR, cvi, ctau, cc, punc, 2);
    m2prob = hashedComputeMod2Prob(mw[i], M[i], mt[i], P, M[chead], mt[chead], mw[chead], frameR, cvi, ctau, cc, punc, 2);
    prob += m1prob + m2prob;

    if( cc ){
      c1prob = hashedComputeCP1Prob(ct, P, M[chead], mt[chead], mw[chead], M[i], mt[i], mw[i], CC);
      c2prob = hashedComputeCP2Prob(cw, ct, P, M[chead], mt[chead], mw[chead], M[i], mt[i], mw[i], CC);
      cprob = c1prob + c2prob;
      prob += cprob;
      cc = ct = cw = 0; 
    }
    if( punc ){
      c1prob = hashedComputeCP1Prob(pt, P, M[chead], mt[chead], mw[chead], M[i], mt[i], mw[i], PUNC);
      c2prob = hashedComputeCP2Prob(pw, pt, P, M[chead], mt[chead], mw[chead], M[i], mt[i], mw[i], PUNC);
      cprob = c1prob + c2prob;
      prob += cprob;
      punc = pt = pw = 0;
    }

    d = OTHER_TAU;
    frameR = removeSubcatI(M[i], frameR);
  }
  if( frameR!=0 )
    { fprintf(stderr, "Warning: Right subcat non-empty: %d\n", frameR); exit(18); }
    
  if( P==labelIndex_NPB ){
    chead = nR;
    cvi = 0; ctau = START_TAU;
  }
  else{
    chead = 0;
    cvi = viStop; ctau = d;
  }
  if( stopFlag==2 || stopFlag==3 ){
    mprob = hashedComputeMod1Prob(0, 0, P, M[chead], mt[chead], mw[chead], frameR, cvi, ctau, 0, 0, 2);  
    prob += mprob;
  }
  
  return prob;
}

double probSBLM2(int arr[], int stopFlag)
{
  double prob=0, hprob, fLprob, fRprob, mprob, m1prob, m2prob, cprob, c1prob, c2prob;
  int i, j, vi[MAXMODS], cvi, viStop, P, t, w, M[MAXMODS], mt[MAXMODS], mw[MAXMODS], frameL=0, frameR=0;
  int parr, cc, ct, cw, punc, pt, pw, d, ctau, nL=0, nR=0, chead, cv;

  parr = 0;
  readNode2(arr, &parr, 1, &P, &t, &w, 0);
  readNode2(arr, &parr, 2, &(M[0]), &(mt[0]), &(mw[0]), 0);

  if( t!=mt[0] || w!=mw[0] ){
    fprintf(stderr, "Error: parent and head nodes must have the same headtag/headword\n"); exit(30);
  }

  hprob = hashedComputeHeadProb(M[0], P, t, w);
  prob += hprob;

  /* left modifiers */
  nL = 1; vi[0] = 0; cv = 0;
  while( 1 ){
    vi[nL] = vi[nL-1] || cv;
    if( !readNode2(arr, &parr, 3, &(M[nL]), &(mt[nL]), &(mw[nL]), &cv) ) break;
    frameL = addSubcatI(M[nL], frameL);
    nL++;
    if( nL >= MAXMODS ){ fprintf(stderr, "Error: maximum number %d of modifiers exceeded\n", MAXMODS); exit(31); }
  }
  viStop = vi[nL-1] || cv;
  nL--;

  if( nlframe[P][M[0]]!=1 ){
    fLprob = hashedComputeFrameProb(frameL, P, M[0], t, w, 1); 
    prob += fLprob;
  }

  d = START_TAU;

  cc = ct = cw = 0; 
  punc = pt = pw = 0;
  for(i=1; i<=nL; i++){
    if( M[i]==labelIndex_Comma || M[i]==labelIndex_Colon ){
      punc = 1;
      pt = mt[i];
      pw = mw[i];
      continue;
    }
    if( P==labelIndex_NPB ){
      for(j=i-1; j>0; j--)
	if( M[j]!=labelIndex_Comma && M[j]!=labelIndex_Colon ) break;
      chead = j;
      cvi = 0; ctau = START_TAU;
    }
    else{
      chead = 0;
      cvi = vi[i]; ctau = d;
    }
	  
    m1prob = hashedComputeMod1Prob(M[i], mt[i], P, M[chead], mt[chead], mw[chead], frameL, cvi, ctau, 0, punc, 1);
    m2prob = hashedComputeMod2Prob(mw[i], M[i], mt[i], P, M[chead], mt[chead], mw[chead], frameL, cvi, ctau, 0, punc, 1);
    prob += m1prob + m2prob;

    if( punc ){
      c1prob = hashedComputeCP1Prob(pt, P, M[chead], mt[chead], mw[chead], M[i], mt[i], mw[i], PUNC);
      c2prob = hashedComputeCP2Prob(pw, pt, P, M[chead], mt[chead], mw[chead], M[i], mt[i], mw[i], PUNC);
      cprob = c1prob + c2prob;
      prob += cprob;
      punc = pt = pw = 0;
    }

    d = OTHER_TAU;
    frameL = removeSubcatI(M[i], frameL);
  }
  if( frameL!=0 )
    { fprintf(stderr, "Warning: Left subcat non-empty: %d\n", frameL); exit(18); }
    
  if( P==labelIndex_NPB ){
    chead = nL;
    cvi = 0; ctau = START_TAU;
  }
  else{
    chead = 0;
    cvi = viStop; ctau = d;
  }
  if( stopFlag==1 || stopFlag==3 ){
    mprob = hashedComputeMod1Prob(0, 0, P, M[chead], mt[chead], mw[chead], frameL, cvi, ctau, 0, punc, 1);
    prob += mprob;
  }

  /* right modifiers */
  nR = 1; vi[0] = 0; cv = 0;
  while( 1 ){
    vi[nR] = vi[nR-1] || cv;
    if( !readNode2(arr, &parr, 3, &(M[nR]), &(mt[nR]), &(mw[nR]), &cv) ) break;
    frameR = addSubcatI(M[nR], frameR);
    nR++;
    if( nR >= MAXMODS ){ fprintf(stderr, "Error: maximum number %d of modifiers exceeded\n", MAXMODS); exit(31); }
  }
  viStop = vi[nR-1] || cv;
  nR--;
  
  if( nrframe[P][M[0]]!=1 ){
    fRprob = hashedComputeFrameProb(frameR, P, M[0], t, w, 2); 
    prob += fRprob;
  }

  d = START_TAU;

  cc = ct = cw = 0; 
  punc = pt = pw = 0;
  for(i=1; i<=nR; i++){
    if( M[i]==labelIndex_CC && M[i]!=labelIndex_NPB ){
      cc = 1;
      ct = mt[i];
      cw = mw[i];
      continue;
    }
    if( M[i]==labelIndex_Comma || M[i]==labelIndex_Colon ){
      punc = 1;
      pt = mt[i];
      pw = mw[i];
      continue;
    }
    if( P==labelIndex_NPB ){
      for(j=i-1; j>0; j--)
	if( M[j]!=labelIndex_Comma && M[j]!=labelIndex_Colon ) break;
      chead = j;
      cvi = 0; ctau = START_TAU;
    }
    else{
      chead = 0;
      cvi = vi[i]; ctau = d;
    }
	  
    m1prob = hashedComputeMod1Prob(M[i], mt[i], P, M[chead], mt[chead], mw[chead], frameR, cvi, ctau, cc, punc, 2);
    m2prob = hashedComputeMod2Prob(mw[i], M[i], mt[i], P, M[chead], mt[chead], mw[chead], frameR, cvi, ctau, cc, punc, 2);
    prob += m1prob + m2prob;

    if( cc ){
      c1prob = hashedComputeCP1Prob(ct, P, M[chead], mt[chead], mw[chead], M[i], mt[i], mw[i], CC);
      c2prob = hashedComputeCP2Prob(cw, ct, P, M[chead], mt[chead], mw[chead], M[i], mt[i], mw[i], CC);
      cprob = c1prob + c2prob;
      prob += cprob;
      cc = ct = cw = 0; 
    }
    if( punc ){
      c1prob = hashedComputeCP1Prob(pt, P, M[chead], mt[chead], mw[chead], M[i], mt[i], mw[i], PUNC);
      c2prob = hashedComputeCP2Prob(pw, pt, P, M[chead], mt[chead], mw[chead], M[i], mt[i], mw[i], PUNC);
      cprob = c1prob + c2prob;
      prob += cprob;
      punc = pt = pw = 0;
    }

    d = OTHER_TAU;
    frameR = removeSubcatI(M[i], frameR);
  }
  if( frameR!=0 )
    { fprintf(stderr, "Warning: Right subcat non-empty: %d\n", frameR); exit(18); }
    
  if( P==labelIndex_NPB ){
    chead = nR;
    cvi = 0; ctau = START_TAU;
  }
  else{
    chead = 0;
    cvi = viStop; ctau = d;
  }
  if( stopFlag==2 || stopFlag==3 ){
    mprob = hashedComputeMod1Prob(0, 0, P, M[chead], mt[chead], mw[chead], frameR, cvi, ctau, cc, punc, 2);  
    prob += mprob;
  }
  
  return prob;
}

int readNode(char buf[], char *info, int *NI, int *tI, int *wI, int *cvI)
{
  int i, flag, nN, nt, nw, ncv;
  char N[SLEN], t[SLEN], w[SLEN], cv[2];

  if( strlen(buf)<5 ) return 0;

  nN = nt = nw = ncv = 0;
  for(i=0, flag=0; i<strlen(buf); i++){
    if( buf[i]=='~' ){
      if( flag==0 ) N[nN] = 0;
      else if( flag==1 ) t[nt] = 0;
      else if( flag==2 ) w[nw] = 0;
      else{ fprintf(stderr, "Error: wrong format for %s: %s\n", info, buf); exit(26); }
      flag++;
      continue;
    }
    if( flag==0 ) N[nN++] = buf[i];
    else if( flag==1 ) t[nt++] = buf[i];
    else if( flag==2 ) w[nw++] = buf[i];
    else if( flag==3 && cvI ) cv[ncv++] = buf[i];
    else{ fprintf(stderr, "Error: wrong format for %s: %s\n", info, buf); exit(27); }
  }
  if( flag==2 ){
    if( !strcmp(info, "parent") || !strcmp(info, "head") )
      w[nw] = 0;
    else{ fprintf(stderr, "Error: wrong format for %s: %s\n", info, buf); exit(28); }
  }
  else if( flag==3 ){
    if( !strcmp(info, "modL") || !strcmp(info, "modR") )
      cv[ncv] = 0;
    else{ fprintf(stderr, "Error: wrong format for %s: %s\n", info, buf); exit(29); }
  }

  if( !strcmp(N, "TOP") ){ strcpy(N, "S1"); }
  *NI = getLabelIndex(N, 0);
  *tI = getLabelIndex(t, 0);
  *wI = getWordIndex(w, 0);
  if( cvI )
    *cvI = atoi(cv);
  return 1;
}

int unknownSBLM(int wI)
{
  return unk[wI];
}


int readNode2(int arr[], int *parr, int type, int *NI, int *tI, int *wI, int *cvI)
{
  if( arr[(*parr)]<0 ){ (*parr)++; return 0; }
  *NI = arr[(*parr)++];
  *tI = arr[(*parr)++];
  *wI = arr[(*parr)++];
  if( type==3 )
    *cvI = arr[(*parr)++];
  return 1;
}

double computePriorProb(int M, int t, int w)
{
  unsigned char joint[SLEN], cond[SLEN];
  double p_0, p_1, p_2, p_3, lambda_0, lambda_1, lambda_2, lambda_3;
  double prior; 
  int jn1=0, cn1=0, jn2, cn2, jn3, cn3, jn4, cn4;

  if( unk[w] ) w = UNKINDEX;

  joint[jn1++] = t; jn2 = jn1; jn1 = writeWord(joint, w, jn1); joint[jn1++] = M; 
   cond[cn1++] = t; cn2 = cn1; cn1 = writeWord( cond, w, cn1); 
  computeProbAndLambda(joint, jn1, cond, cn1, 71, 72, 73, 0, 0, FFACTOR, &p_1, &lambda_1, 0, 0, 0);
  
  joint[jn2++] = M; 
  computeProbAndLambda(joint, jn2, cond, cn2, 74, 75, 76, 0, 0, FFACTOR, &p_2, &lambda_2, 0, 0, 0);

  jn3 = cn3 = 0;
  joint[jn3++] = LBL; joint[jn3++] = M;
   cond[cn3++] = LBL;
  computeProbAndLambda(joint, jn3, cond, cn3, 77, 78, 79, 0, 0, FFACTOR, &p_3, &lambda_3, 0, 0, 0);
  
  jn4 = cn4 = 0;
  joint[jn4++] = TW; joint[jn4++] = t; jn4 = writeWord(joint, w, jn4);
   cond[cn4++] = TW;
  computeProbAndLambda(joint, jn4, cond, cn4, 81, 82, 83, 0, 1, 0, &p_0, &lambda_0, 0, 0, 0);
  if( IRENEHACK && w==UNKINDEX )
    p_0 /= (tagNUNK[t]+1);

  prior = lambda_1*p_1 + (1-lambda_1)*(lambda_2*p_2 + (1-lambda_2)*(lambda_3*p_3 + (1-lambda_3)*epsilonP));
  prior *= p_0;

  return log(prior);
}

double computeHeadProb(int H, int P, int t, int w, int logFlag)
{
  double p=0, p_1=0, p_2=0, p_3=0, p_4=0, p_5=0, p_0=0;
  double lambda_1=0, lambda_2=0, lambda_3=0, lambda_4=0, lambda_5=0, lambda_0=0; 
  unsigned char joint[SLEN], cond[SLEN];
  int jn1, cn1, jn2, cn2, jn3, cn3, jn4, cn4;
  
  if( SBLM )
    if( unk[w] ) w = UNKINDEX;

  if( P != labelIndex_S1 ){
    jn1 = cn1 = 0;
    joint[jn1++] = P; jn3 = jn1; joint[jn1++] = t; jn2 = jn1; jn1 = writeWord(joint, w, jn1); joint[jn1++] = H; 
     cond[cn1++] = P; cn3 = cn1;  cond[cn1++] = t; cn2 = cn1; cn1 = writeWord( cond, w, cn1);  
    computeProbAndLambda(joint, jn1, cond, cn1, 1, 2, 3, 0, 0, FFACTOR, &p_1, &lambda_1, 0, 0, 0);

    joint[jn2++] = H; 
    computeProbAndLambda(joint, jn2, cond, cn2, 4, 5, 6, 0, 0, FFACTOR, &p_2, &lambda_2, 0, 0, 0);

    joint[jn3++] = H; 
    computeProbAndLambda(joint, jn3, cond, cn3, 7, 8, 9, 0, 0, FFACTOR, &p_3, &lambda_3, 0, 0, 0);

    jn4 = 0; cn4 = 0;
    joint[jn4++] = HEAD; joint[jn4++] = H;
     cond[cn4++] = HEAD;
    computeProbAndLambda(joint, jn4, cond, cn4, 7, 8, 9, 0, 0, 0, &p_4, &lambda_4, 0, 0, 0);

    lambda_5 = 1;
    p_5 = 1.0/NNTS;
 
    p = smoothProb(lambda_1, p_1, lambda_2, p_2, lambda_3, p_3, lambda_4, p_4, lambda_5, p_5); 
  }
  else{ /* TOP probability */
    jn1 = cn1 = 0;
    joint[jn1++] = P; joint[jn1++] = H; joint[jn1++] = t;
     cond[cn1++] = P;
    computeProbAndLambda(joint, jn1, cond, cn1, 61, 62, 0, 0, 0, 0, &p_0, &lambda_0, 0, 0, 0);

    jn2 = cn2 = 0;
    joint[jn2++] = P; joint[jn2++] = H; joint[jn2++] = t; jn2 = writeWord(joint, w, jn2); 
     cond[cn2++] = P;  cond[cn2++] = H;  cond[cn2++] = t; 
    computeProbAndLambda(joint, jn2, cond, cn2, 64, 65, 66, 0, 0, FFACTOR_M2, &p_1, &lambda_1, 0, 0, 0);
    
    jn3 = cn3 = 0;
    joint[jn3++] = t; jn3 = writeWord(joint, w, jn3); 
     cond[cn3++] = t;
    /* this is computed with both left and right counts */
    /* this is P(w|t) in the space of lexicalized preterminals */
    computeProbAndLambda(joint, jn3, cond, cn3, 37, 38, 39, 0, 0, FFACTOR, &p_2, &lambda_2, 0, 0, 0);

    p_3 = p_4 = 0;
    lambda_3 = lambda_4 = 0;

    p = smoothProb(lambda_1, p_1, lambda_2, p_2, lambda_3, p_3, lambda_4, p_4, 0, 0); 
    if( IRENEHACK && w==UNKINDEX )
      p /= NUNK;
    p *= p_0;
  }

  if( Debug )
    printf("\t P_h(%s | %s %s %s) = %f %f\n", labelIndex[H], labelIndex[P], labelIndex[t], wordIndex[w], log(p), p);

  if( logFlag )
    return log(p);
  else
    return p;
}

double computeFrameProb(int C, int P, int H, int t, int w, int side, int logFlag)
{
  unsigned char joint[SLEN], cond[SLEN];
  double p=0, p_1=0, p_2=0, p_3=0, p_4=0, p_5=0;
  double lambda_1=0, lambda_2=0, lambda_3=0, lambda_4=0, lambda_5=0; 
  int jn1, cn1, jn2, cn2, jn3, cn3, jn4, cn4;

  if( !SBLM )
    P = rmC[P];
  if( SBLM ) 
    if( unk[w] ) w = UNKINDEX;

  jn1 = cn1 = 0;

  joint[jn1++] = P; joint[jn1++] = H; jn3 = jn1; joint[jn1++] = t; jn2 = jn1; jn1 = writeWord(joint, w, jn1); jn1 = writeFrame(joint, C, jn1);
   cond[cn1++] = P;  cond[cn1++] = H; cn3 = cn1;  cond[cn1++] = t; cn2 = cn1; cn1 = writeWord( cond, w, cn1); 
  computeProbAndLambda(joint, jn1, cond, cn1, 11, 12, 0, side, FTERM, 0, &p_1, &lambda_1, 0, 0, 0);

  jn2 = writeFrame(joint, C, jn2);
  computeProbAndLambda(joint, jn2, cond, cn2, 14, 15, 0, side, FTERM, 0, &p_2, &lambda_2, 0, 0, 0);

  jn3 = writeFrame(joint, C, jn3);
  computeProbAndLambda(joint, jn3, cond, cn3, 17, 18, 0, side, FTERM, 0, &p_3, &lambda_3, 0, 0, 0);

  jn4 = 0; cn4 = 0;
  joint[jn4++] = FRAME; joint[jn4++] = C;
   cond[cn4++] = FRAME;
  computeProbAndLambda(joint, jn4, cond, cn4, 17, 18, 0, side, FTERM, 0, &p_4, &lambda_4, 0, 0, 0);
  
  lambda_5 = 1;
  p_5 = 1.0/NFRS;

  p = smoothProb(lambda_1, p_1, lambda_2, p_2, lambda_3, p_3, lambda_4, p_4, lambda_5, p_5); 

  if( Debug )
    printf("\t P_f(%d | %s %s %s %s %d) = %f %f\n", C, labelIndex[P], labelIndex[H], labelIndex[t], wordIndex[w], side, log(p), p);

  if( logFlag )
    return log(p);
  else
    return p;
}

double computeModProb(int M, int mt, int mw, int P, int H, int t, int w, int C, int V, int T, int cc, int punc, int side, int logFlag)
{
  double prob, mod1prob, mod2prob;

  mod1prob = computeMod1Prob(M, mt, P, H, t, w, C, V, T, cc, punc, side, logFlag);
  prob = mod1prob;
  if( M ){
    mod2prob = computeMod2Prob(mw, M, mt, P, H, t, w, C, V, T, cc, punc, side, logFlag);
    if( logFlag )
      prob += mod2prob;
    else
      prob *= mod2prob;
  }
  return prob;
}

double computeMod1Prob(int M, int mt, int P, int H, int t, int w, int C, int V, int T, int cc, int punc, int side, int logFlag)
{
  unsigned char cond[SLEN], joint[SLEN];
  double p[6], lambda[6], prob; 
  int i, jn[6], cn[6];
  int cjoint[6], ccond[6], cu[6];

  if( P==labelIndex_S1 && M==0 )
    if( logFlag ) return 0;
    else return 1;

  if( !SBLM )
    P = rmC[P];
  if( SBLM ) 
    if( unk[w] ) w = UNKINDEX;

  jn[1] = cn[1] = 0;
  joint[jn[1]++] = P; jn[1] = writeFrame(joint, C, jn[1]); joint[jn[1]++] = V; joint[jn[1]++] = T; 
   cond[cn[1]++] = P; cn[1] = writeFrame( cond, C, cn[1]);  cond[cn[1]++] = V;  cond[cn[1]++] = T;  
  joint[jn[1]++] = H; jn[3] = jn[1]; joint[jn[1]++] = t; jn[2] = jn[1]; jn[1] = writeWord(joint, w, jn[1]); 
   cond[cn[1]++] = H; cn[3] = cn[1];  cond[cn[1]++] = t; cn[2] = cn[1]; cn[1] = writeWord( cond, w, cn[1]); 
  
  joint[jn[1]++] = M; joint[jn[1]++] = mt; joint[jn[1]++] = cc; joint[jn[1]++] = punc; 
  computeProbAndLambda(joint, jn[1], cond, cn[1], 21, 22, 23, side, 0, FFACTOR, &p[1], &lambda[1], &cjoint[1], &ccond[1], &cu[1]);
  
  joint[jn[2]++] = M; joint[jn[2]++] = mt; joint[jn[2]++] = cc; joint[jn[2]++] = punc; 
  computeProbAndLambda(joint, jn[2], cond, cn[2], 24, 25, 26, side, 0, FFACTOR, &p[2], &lambda[2], &cjoint[2], &ccond[2], &cu[2]);

  joint[jn[3]++] = M; joint[jn[3]++] = mt; joint[jn[3]++] = cc; joint[jn[3]++] = punc; 
  computeProbAndLambda(joint, jn[3], cond, cn[3], 27, 28, 29, side, 0, FFACTOR, &p[3], &lambda[3], &cjoint[3], &ccond[3], &cu[3]);

  jn[4] = 0; cn[4] = 0;
  joint[jn[4]++] = MOD1; joint[jn[4]++] = M; joint[jn[4]++] = mt; joint[jn[4]++] = cc; joint[jn[4]++] = punc; 
   cond[cn[4]++] = MOD1;
  computeProbAndLambda(joint, jn[4], cond, cn[4], 27, 28, 29, side, 0, FFACTOR, &p[4], &lambda[4], &cjoint[4], &ccond[4], &cu[4]);

  lambda[5] = 1;
  cjoint[5] = 1; ccond[5] = NNTS*NPTS*4 + 1; /* 4 for |cc|*|punc|; 1 for +STOP+ */
  cu[5] = NNTS*NPTS*4 + 1;
  p[5] = cjoint[5]/(double)ccond[5];

  prob = smoothProb(lambda[1], p[1], lambda[2], p[2], lambda[3], p[3], lambda[4], p[4], lambda[5], p[5]); 
  if( Debug ){
    printf("\t P_m1(%s %s %d %d | %s %s %s %s %d %d %d %d) = %f %f\n", labelIndex[M], labelIndex[mt], 0, punc, labelIndex[P], labelIndex[H], labelIndex[t], wordIndex[w], C, V, T, side, log(prob), prob);
    for(i=1; i<=5; i++){
      if( smoothLevel==4 && i==4 ) continue;
      if( lambda[i]==0 && p[i]==0 ) continue;
      printf("\t\t %d : (%.3f)*%.6f : (%d futures) %d/%d\n", i, lambda[i], p[i], cu[i], cjoint[i], ccond[i]);
    }
  }
  if( logFlag )
    return log(prob);
  else
    return prob;
}

double computeMod2Prob(int mw, int M, int mt, int P, int H, int t, int w, int C, int V, int T, int cc, int punc, int side, int logFlag)
{
  unsigned char joint[SLEN], cond[SLEN];
  double p[6], lambda[6], prob;
  int i, jn[6], cn[6];
  int cjoint[6], ccond[6], cu[6];

  if( !SBLM )
    P = rmC[P];
  if( SBLM ) 
    if( unk[w] ) w = UNKINDEX;

  if( unk[mw] ) mw = UNKINDEX;

  jn[1] = cn[1] = 0;
  joint[jn[1]++] = P; jn[1] = writeFrame(joint, C, jn[1]); joint[jn[1]++] = V; joint[jn[1]++] = T; 
   cond[cn[1]++] = P; cn[1] = writeFrame( cond, C, cn[1]);  cond[cn[1]++] = V;  cond[cn[1]++] = T;  
  joint[jn[1]++] = H; joint[jn[1]++] = t; jn[2] = jn[1]; jn[1] = writeWord(joint, w, jn[1]); 
   cond[cn[1]++] = H;  cond[cn[1]++] = t; cn[2] = cn[1]; cn[1] = writeWord( cond, w, cn[1]); 
  joint[jn[1]++] = M; joint[jn[1]++] = mt; joint[jn[1]++] = cc; joint[jn[1]++] = punc; 
   cond[cn[1]++] = M;  cond[cn[1]++] = mt;  cond[cn[1]++] = cc;  cond[cn[1]++] = punc;

  jn[1] = writeWord(joint, mw, jn[1]); 
  computeProbAndLambda(joint, jn[1], cond, cn[1], 31, 32, 33, side, 0, FFACTOR_M2, &p[1], &lambda[1], &cjoint[1], &ccond[1], &cu[1]);

  joint[jn[2]++] = M; joint[jn[2]++] = mt; joint[jn[2]++] = cc; joint[jn[2]++] = punc; 
   cond[cn[2]++] = M;  cond[cn[2]++] = mt;  cond[cn[2]++] = cc;  cond[cn[2]++] = punc; 

  jn[2] = writeWord(joint, mw, jn[2]); 
  computeProbAndLambda(joint, jn[2], cond, cn[2], 34, 35, 36, side, 0, FFACTOR_M2, &p[2], &lambda[2], &cjoint[2], &ccond[2], &cu[2]);

  jn[3] = cn[3] = 0;
  joint[jn[3]++] = mt; jn[3] = writeWord(joint, mw, jn[3]);
   cond[cn[3]++] = mt;
  computeProbAndLambda(joint, jn[3], cond, cn[3], 37, 38, 39, 0, 0, FFACTOR_M2, &p[3], &lambda[3], &cjoint[3], &ccond[3], &cu[3]);

  jn[4] = cn[4] = 0;
  joint[jn[4]++] = MOD2; jn[4] = writeWord(joint, mw, jn[4]);
   cond[cn[4]++] = MOD2;
  computeProbAndLambda(joint, jn[4], cond, cn[4], 37, 38, 39, 0, 0, FFACTOR_M2, &p[4], &lambda[4], &cjoint[4], &ccond[4], &cu[4]);

  lambda[5] = 1;
  cjoint[5] = 1; ccond[5] = NLEX+1; /* 1 for +UNK+ */
  cu[5] = NLEX+1; 
  p[5] = cjoint[5]/(double)ccond[5];

  prob = smoothProb(lambda[1], p[1], lambda[2], p[2], lambda[3], p[3], lambda[4], p[4], lambda[5], p[5]); 
  if( IRENEHACK && mw==UNKINDEX )
    prob /= NUNK;
  
  if( Debug ){
    printf("\t P_m2(%s | %s %s %s %s %s %s %d %d %d %d %d %d) = %f %f\n", wordIndex[mw], labelIndex[M], labelIndex[mt], labelIndex[P], labelIndex[H], labelIndex[t], wordIndex[w], C, V, T, 0, punc, side, log(prob), prob);
    for(i=1; i<=5; i++){
      if( smoothLevel==4 && i==4 ) continue;
      if( lambda[i]==0 && p[i]==0 ) continue;
      printf("\t\t %d : (%.3f)*%.6f : (%d futures) %d/%d\n", i, lambda[i], p[i], cu[i], cjoint[i], ccond[i]);
    }
  }
  
  if( logFlag )
    return log(prob);
  else
    return prob;
}

double computeCPProb(int t, int w, int P, int H1, int t1, int w1, int H2, int t2, int w2, int type, int logFlag)
{
  double prob, cp1prob, cp2prob;

  cp1prob = computeCP1Prob(t, P, H1, t1, w1, H2, t2, w2, type, logFlag);
  cp2prob = computeCP2Prob(w, t, P, H1, t1, w1, H2, t2, w2, type, logFlag);
  if( logFlag )
    prob = cp1prob + cp2prob;
  else
    prob = cp1prob * cp2prob;
    
  if( Debug )
    printf("\t P_cp(%s %s | %s %s %s %s %s %s %s %d) = %f %f\n", labelIndex[t], wordIndex[w], labelIndex[P], labelIndex[H1], labelIndex[t1], wordIndex[w1], labelIndex[H2], labelIndex[t2], wordIndex[w2], type, log(prob), prob);
  
  return prob;
}

double computeCP1Prob(int ct, int P, int H, int t, int w, int M, int mt, int mw, int type, int logFlag)
{
  unsigned char joint[SLEN], cond[SLEN];
  double p=0, p_1=0, p_2=0, p_3=0, p_5=0;
  double lambda_1=0, lambda_2=0, lambda_3=0, lambda_5=0; 
  int jn1, cn1, jn2, cn2, jn3, cn3;
  
  if( !SBLM )
    P = rmC[P];
  if( SBLM ){
    if( unk[w] ) w = UNKINDEX;
    if( unk[mw] ) mw = UNKINDEX;
  }

  jn1 = cn1 = 0;
  joint[jn1++] = P; joint[jn1++] = H; joint[jn1++] =  t; jn1 = writeWord(joint,  w, jn1); 
                    joint[jn1++] = M; joint[jn1++] = mt; jn1 = writeWord(joint, mw, jn1);
		    joint[jn1++] = type;
   cond[cn1++] = P;  cond[cn1++] = H;  cond[cn1++] =  t; cn1 = writeWord( cond,  w, cn1); 
                     cond[cn1++] = M;  cond[cn1++] = mt; cn1 = writeWord( cond, mw, cn1); 
		     cond[cn1++] = type;

  joint[jn1++] = ct;		     
  computeProbAndLambda(joint, jn1, cond, cn1,  41,  42,  43, 0, 0, FFACTOR, &p_1, &lambda_1, 0, 0, 0);
    
  jn2 = cn2 = 0;
  joint[jn2++] = P; joint[jn2++] = H; joint[jn2++] = t; joint[jn2++] = M; joint[jn2++] = mt; 
  joint[jn2++] = type;
   cond[cn2++] = P;  cond[cn2++] = H;  cond[cn2++] = t;  cond[cn2++] = M;  cond[cn2++] = mt;      
   cond[cn2++] = type;

  joint[jn2++] = ct;
  computeProbAndLambda(joint, jn2, cond, cn2,  44,  45,  46, 0, 0, FFACTOR, &p_2, &lambda_2, 0, 0, 0);

  jn3 = cn3 = 0;
  joint[jn3++] = type; joint[jn3++] = ct;
   cond[cn3++] = type;
  computeProbAndLambda(joint, jn3, cond, cn3,  47,  48,  49, 0, 0, FFACTOR, &p_3, &lambda_3, 0, 0, 0);

  lambda_5 = 1;
  p_5 = 1/NPTS;

  p = smoothProb(lambda_1, p_1, lambda_2, p_2, lambda_3, p_3, 0, 0, lambda_5, p_5); 

  if( logFlag )
    return log(p);
  else
    return p;
}

double computeCP2Prob(int cw, int ct, int P, int H, int t, int w, int M, int mt, int mw, int type, int logFlag)
{
  unsigned char joint[SLEN], cond[SLEN];
  double p=0, p_1=0, p_2=0, p_3=0, p_4=0, p_5=0;
  double lambda_1=0, lambda_2=0, lambda_3=0, lambda_4=0, lambda_5=0; 
  int jn1, cn1, jn2, cn2, jn3, cn3, jn4, cn4;
  
  if( !SBLM )
    P = rmC[P]; 
  if( SBLM ){
    if( unk[w] ) w = UNKINDEX;
    if( unk[mw] ) mw = UNKINDEX;
  }
  
  if( unk[cw] ) cw = UNKINDEX;

  jn1 = cn1 = 0;
  joint[jn1++] = P; joint[jn1++] = H; joint[jn1++] =  t; jn1 = writeWord(joint,  w, jn1); 
                    joint[jn1++] = M; joint[jn1++] = mt; jn1 = writeWord(joint, mw, jn1); joint[jn1++] = ct;
		    joint[jn1++] = type;
   cond[cn1++] = P;  cond[cn1++] = H;  cond[cn1++] =  t; cn1 = writeWord( cond,  w, cn1); 
                     cond[cn1++] = M;  cond[cn1++] = mt; cn1 = writeWord( cond, mw, cn1);  cond[cn1++] = ct;
		     cond[cn1++] = type;

  jn1 = writeWord(joint, cw, jn1); 
  computeProbAndLambda(joint, jn1, cond, cn1,  51,  52,  53, 0, 0, FFACTOR_M2, &p_1, &lambda_1, 0, 0, 0);

  jn2 = cn2 = 0;
  joint[jn2++] = P; joint[jn2++] = H; joint[jn2++] = t; joint[jn2++] = M; joint[jn2++] = mt; joint[jn2++] = ct;
  joint[jn2++] = type;
   cond[cn2++] = P;  cond[cn2++] = H;  cond[cn2++] = t;  cond[cn2++] = M;  cond[cn2++] = mt;  cond[cn2++] = ct;
   cond[cn2++] = type;

  jn2 = writeWord(joint, cw, jn2); 
  computeProbAndLambda(joint, jn2, cond, cn2,  54,  55,  56, 0, 0, FFACTOR_M2, &p_2, &lambda_2, 0, 0, 0);

  /* this is P(w|t) estimation in the space of lexicalized preterminals */
  jn3 = cn3 = 0;
  joint[jn3++] = ct; jn3 = writeWord(joint, cw, jn3); 
   cond[cn3++] = ct;
  computeProbAndLambda(joint, jn3, cond, cn3, 37, 38, 39, 0, 0, FFACTOR_M2, &p_3, &lambda_3, 0, 0, 0);

  jn4 = cn4 = 0;
  joint[jn4++] = MOD2; jn4 = writeWord(joint, cw, jn4);
   cond[cn4++] = MOD2;
  computeProbAndLambda(joint, jn4, cond, cn4, 37, 38, 39, 0, 0, FFACTOR_M2, &p_4, &lambda_4, 0, 0, 0);

  lambda_5 = 1;
  p_5 = 1.0/(NLEX+1); /* 1 for +UNK+ */
 
  p = smoothProb(lambda_1, p_1, lambda_2, p_2, lambda_3, p_3, lambda_4, p_4, lambda_5, p_5); 
  if( IRENEHACK && cw==UNKINDEX )
    p /= NUNK;

  if( logFlag )
    return log(p);
  else
    return p;
}

void updateCnt(unsigned char joint[], int nj, unsigned char cond[], int nc, int cnt, int jointFlag, int condFlag, int side)
{
  Key k;

  if( nj>0 ){
    joint[nj] = jointFlag; joint[nj+1] = side+10;
    k.key = joint; k.len = nj+2;
    addElementJmpHash(&k, &jmphashCnt, cnt);
  }

  if( nc>0 ){
    cond[nc] = condFlag; cond[nc+1] = side+10;
    k.key = cond; k.len = nc+2;
    addElementJmpHash(&k, &jmphashCnt, cnt);
  }
}

void updateCntAndOutcome(unsigned char joint[], int nj, unsigned char cond[], int nc, int cnt, int jointFlag, int condFlag, int outcomeFlag, int side)
{
  Key k;
  int c, newOutcome=0;
  
  joint[nj] = jointFlag; joint[nj+1] = side+10;
  k.key = joint; k.len = nj+2;
  c = addElementJmpHash(&k, &jmphashCnt, cnt);
  if( c == cnt ) /* new outcome on cond */
    newOutcome = 1;

  cond[nc] = condFlag; cond[nc+1] = side+10;
  k.key = cond; k.len = nc+2;
  addElementJmpHash(&k, &jmphashCnt, cnt);
  if( newOutcome ){
    cond[nc] = outcomeFlag;
    k.key = cond; k.len = nc+2;
    addElementJmpHash(&k, &jmphashCnt, 1);
  }
}

void setCntAndOutcome(char type, unsigned char joint[], int nj, int jcnt, unsigned char cond[], int nc, int ccnt, int ucnt, int jointFlag, int condFlag, int outcomeFlag, int side)
{
  Key k;

  joint[nj] = jointFlag; joint[nj+1] = side+10;
  k.key = joint; k.len = nj+2;
  setElementJmpHash(&k, &jmphashCnt, jcnt);

  cond[nc] = condFlag; cond[nc+1] = side+10;
  k.key = cond; k.len = nc+2;
  setElementJmpHash(&k, &jmphashCnt, ccnt);
  
  if( outcomeFlag ){
    cond[nc] = outcomeFlag;
    k.key = cond; k.len = nc+2;
    setElementJmpHash(&k, &jmphashCnt, ucnt);
  }
}

void computeProbAndLambda(unsigned char joint[], int nj, unsigned char cond[], int nc, int jointFlag, int condFlag, int outcomeFlag, int side, int fterm, int ffactor, double *p, double *l, int *cjoint, int *ccond, int *cu)
{
  int c1, c2, u=0;
  Key k;

  *p = *l = 0;

  joint[nj] = jointFlag; joint[nj+1] = side+10;
  k.key = joint; k.len = nj+2;
  c1 = findElementJmpHash(&k, &jmphashCnt);
  if( cjoint ) *cjoint = c1;

  cond[nc] = condFlag; cond[nc+1] = side+10;
  k.key = cond; k.len = nc+2;
  c2 = findElementJmpHash(&k, &jmphashCnt);
  if( ccond ) *ccond = c2;
  if( c1 && c2 )
    *p = c1/(double)c2;
  else *p = 0;

  if( outcomeFlag ){
    cond[nc] = outcomeFlag; 
    k.key = cond; k.len = nc+2;
    u = findElementJmpHash(&k, &jmphashCnt);
    if( cu ) *cu = u;
  }
  else
    if( cu ) *cu = 0;

  if( c2 ) 
    *l = c2/(double)(c2+fterm+ffactor*u);
  else 
    *l = 0;
}

double smoothProb(double lambda_1, double p_1, double lambda_2, double p_2, double lambda_3, double p_3, double lambda_4, double p_4, double lambda_5, double p_5)
{
  double p;

  if( smoothLevel==4 ){
    lambda_4 = p_4 = 0; /* delete the prior on event */
  }
  if( !SBLM ){
    lambda_5 = p_5 = 0; /* delete the uniform distr. */
  }

  p = lambda_1*p_1 + (1-lambda_1)*(lambda_2*p_2 + (1-lambda_2)*(lambda_3*p_3 + (1-lambda_3)*(lambda_4*p_4 + (1-lambda_4)*(lambda_5*p_5 + (1-lambda_5)*epsilonP))));
  return p;
}

int getSide(char *side)
{
  if( !strcmp(side, "+m+") ) return 0;
  if( !strcmp(side, "+l+") ) return 1;
  if( !strcmp(side, "+r+") ) return 2;
  assert(0);
  return -1;
}

int getTau(char *tau)
{
  if( !strcmp(tau, "+START+") ) return START_TAU;
  if( !strcmp(tau, "+OTHER+") ) return OTHER_TAU;
  assert(0);
  return -1;
}

/*
void printHashDensity(Hash *hash, char *name)
{
  double d;

  d = hash->total/(double)hash->size;
  if( SHOWHASHDENSITY )
    fprintf(stderr, "Density on %15s: %.2f (%d in %d)\n", name, d, hash->total, hash->size);
  else
    if( d>1 )
      fprintf(stderr, "Warning: Density on %15s: %.2f (%d in %d)\n", name, d, hash->total, hash->size);
      
}
*/

int removeSubcat(char *label, int subcat)
{
  int pos, posval;

  if( label[strlen(label)-1] == 'C' && label[strlen(label)-2] == '-' ){
    pos = mapSubcat(label);
    posval =  (int)pow(10, pos);
    if( (subcat/posval)%10 > 0 )
      subcat -= posval;
    else 
      return -1;
  }
  return subcat;
}

int removeSubcatI(int labelI, int subcat)
{
  char *label;
  
  label = labelIndex[labelI];
  return removeSubcat(label, subcat);
}

int addSubcat(char *label, int subcat)
{
  int pos;

  if( label[strlen(label)-1] == 'C' && label[strlen(label)-2] == '-' ){
    pos = mapSubcat(label);
    subcat += (int)pow(10, pos);
  }
  return subcat;
}

int addSubcatI(int labelI, int subcat)
{
  char *label;

  label = labelIndex[labelI];
  return addSubcat(label, subcat);
}

