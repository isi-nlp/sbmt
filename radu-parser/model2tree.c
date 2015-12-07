#include "model2tree.h"

double model2Prob(Tree *tree, int *wcnt, double *wp, char wbuf[])
{
  double prob, cprob, hprob, fLprob, fRprob, mprob, m1prob, m2prob, pcprob, pc1prob, pc2prob;
  int i, j, vi[100], cvi, pvi, viStop, P, H, t, w, M, mt, mw, cc, ct, cw, punc, pt, pw, d, ctau;
  Tree *t1, *tt, *chead;
  int frameL=0, frameR=0;
  char lex[80], tlex[80], buf[200];

  if( !tree )
    return 0;

  if( !tree->child )
    return 0;

  prob = 0; cprob = 0;
  for(t1=tree->child; t1!=NULL; t1=t1->siblingR)
    cprob += model2Prob(t1, wcnt, wp, wbuf);

  if( tree->child ){
    getLex(lex, tree->lex, 0);

    H = getLabelIndex(tree->headchild->synT, 0); 
    P = getLabelIndex(tree->synT, 0); t = getLabelIndex(tree->preT, 0); 
    w = getWordIndex(lex, 0); 

    hprob = hashedComputeHeadProb(H, P, t, w);
    prob += hprob; 
    if( P == labelIndex_S1 ){
      *wp += hprob; *wcnt += 1;
      sprintf(buf, "p(%s|%s)=%.2f", wordIndex[w], labelIndex[P], hprob);
      strcat(wbuf, buf); strcat(wbuf, " ");
    }

    for(t1=tree->headchild->siblingL; t1!=NULL; t1=t1->siblingL)
      frameL = addSubcat(t1->synT, frameL);

    for(t1=tree->headchild->siblingR; t1!=NULL; t1=t1->siblingR)
      frameR = addSubcat(t1->synT, frameR);

    if( nlframe[P][H]!=1 ){
      fLprob = hashedComputeFrameProb(frameL, P, H, t, w, 1); 
      prob += fLprob;
    }

    if( nrframe[P][H]!=1 ){
      fRprob = hashedComputeFrameProb(frameR, P, H, t, w, 2); 
      prob += fRprob;
    }

    /* left modifiers */
    for(t1=tree->headchild,i=0; t1!=NULL; t1=t1->siblingL,i++){
      if( i<=1 ) vi[i] = 0;
      else{ vi[i] = vi[i-1] || t1->siblingR->cv; }
    }
    if( tree->child==tree->headchild)
      viStop = 0;
    else
      viStop = vi[i-1] || tree->child->cv;

    d = START_TAU;

    cc = ct = cw = 0; 
    punc = pt = pw = 0;
    for(t1=tree->headchild,i=0; t1!=NULL; t1=t1->siblingL,i++){
      if( i==0 ) continue;
      if( !strcmp(t1->synT, ",") || !strcmp(t1->synT, ":") ){
	punc = 1;
	pt = getLabelIndex(t1->synT, 0); 
	getLex(lex, t1->lex, 0); 
	pw = getWordIndex(lex, 0);
	continue;
      }

      if( !strcmp(tree->synT, "NPB") ){
	for(tt=t1->siblingR; tt!=tree->headchild; tt=tt->siblingR)
	  if( strcmp(tt->synT, ",") && strcmp(tt->synT, ":") ) break;
	chead = tt; 
	cvi = 0; ctau = START_TAU;
      }
      else{
	chead = tree->headchild;
	cvi = vi[i]; ctau = d;
      }

      getLex(lex, chead->lex, 0); 
      getLex(tlex, t1->lex, 0);

      H = getLabelIndex(chead->synT, 0); t = getLabelIndex(chead->preT, 0);
      M = getLabelIndex(t1->synT, 0); mt = getLabelIndex(t1->preT, 0); 
      
      w = getWordIndex(lex, 0);
     mw = getWordIndex(tlex, 0);

      m1prob = hashedComputeMod1Prob(M, mt, P, H, t, w, frameL, cvi, ctau, 0, punc, 1);
      m2prob = hashedComputeMod2Prob(mw, M, mt, P, H, t, w, frameL, cvi, ctau, 0, punc, 1);
      mprob = m1prob + m2prob;
      prob += mprob;
      *wp += mprob; *wcnt += 1;
      sprintf(buf, "p(%s|%s)=%.2f", wordIndex[mw], wordIndex[w], mprob);
      strcat(wbuf, buf); strcat(wbuf, " ");

      if( punc !=0 ){/* it seems that pre-head punc *is* generated */ 
	pc1prob = hashedComputeCP1Prob(pt, P, H, t, w, M, mt, mw, PUNC);
	pc2prob = hashedComputeCP2Prob(pw, pt, P, H, t, w, M, mt, mw, PUNC);
	pcprob = pc1prob + pc2prob;
	prob += pcprob;
	*wp += pcprob; *wcnt += 1;
	sprintf(buf, "p(%s|%s_%s)=%.2f", wordIndex[pw], wordIndex[w], wordIndex[mw], pcprob);
	strcat(wbuf, buf); strcat(wbuf, " ");
	punc = pt = pw = 0;
     }

      d = OTHER_TAU;
      frameL = removeSubcat(t1->synT, frameL);
    }
    if( frameL!=0 )
      { fprintf(stderr, "Warning: Left subcat non-empty: %d\n", frameL); exit(18); }
    
    if( !strcmp(tree->synT, "NPB") ){
      chead = tree->child; 
      cvi = 0; ctau = START_TAU;
    }
    else{
      chead = tree->headchild; 
      cvi = viStop; ctau = d;
    }

    getLex(lex, chead->lex, 0);
    H = getLabelIndex(chead->synT, 0); t = getLabelIndex(chead->preT, 0); 
    w = getWordIndex(lex, 0);

    prob += hashedComputeMod1Prob(0, 0, P, H, t, w, frameL, cvi, ctau, 0, punc, 1);

    /* right modifiers */
    for(t1=tree->headchild,i=0; t1!=NULL; t1=t1->siblingR,i++){
      if( i<=1 ) vi[i] = 0;
      else{ vi[i] = vi[i-1] || t1->siblingL->cv; }
    }
    for(tt=tree->headchild; tt->siblingR!=NULL; tt=tt->siblingR) ; /* last child */
    if( tree->headchild==tt ) 
      viStop = 0;
    else 
      viStop = vi[i-1] || tt->cv; 

    d = START_TAU;

    cc = ct = cw = 0; 
    punc = pt = pw = 0;
    for(t1=tree->headchild,i=0; t1!=NULL; t1=t1->siblingR,i++){
      if( i==0 ) continue;
      if( !strcmp(t1->synT, "CC") && strcmp(tree->synT, "NPB") ){
	cc = 1;
	ct = getLabelIndex(t1->synT, 0); 
	getLex(lex, t1->lex, 0);
	cw = getWordIndex(lex, 0);
	continue;
      }
      if( !strcmp(t1->synT, ",") || !strcmp(t1->synT, ":") ){
	punc = 1;
	pt = getLabelIndex(t1->synT, 0);
	getLex(lex, t1->lex, 0); 
	pw = getWordIndex(lex, 0);
	continue;
      }

      if( !strcmp(tree->synT, "NPB") ){
	for(tt=t1->siblingL;tt!=tree->headchild;tt=tt->siblingL)
	  if( strcmp(tt->synT, ",") && strcmp(tt->synT, ":") ) break;
	chead = tt; 
	cvi = 0; ctau = START_TAU;
      }
      else{
	chead = tree->headchild; cvi = vi[i]; ctau = d;
      }

      getLex(lex, chead->lex, 0);
      getLex(tlex, t1->lex, 0);

      H = getLabelIndex(chead->synT, 0); t = getLabelIndex(chead->preT, 0); 
      M = getLabelIndex(t1->synT, 0); mt = getLabelIndex(t1->preT, 0); 

      w = getWordIndex(lex, 0);
      mw = getWordIndex(tlex, 0);     

      m1prob = hashedComputeMod1Prob(M, mt, P, H, t, w, frameR, cvi, ctau, cc, punc, 2);
      m2prob = hashedComputeMod2Prob(mw, M, mt, P, H, t, w, frameR, cvi, ctau, cc, punc, 2);
      mprob = m1prob + m2prob;
      prob += mprob;
      *wp += mprob; *wcnt += 1;
      sprintf(buf, "p(%s|%s)=%.2f", wordIndex[mw], wordIndex[w], mprob);
      strcat(wbuf, buf); strcat(wbuf, " ");

      if( cc != 0 ){
	pc1prob = hashedComputeCP1Prob(ct, P, H, t, w, M, mt, mw, CC);
	pc2prob = hashedComputeCP2Prob(cw, ct, P, H, t, w, M, mt, mw, CC);
	pcprob = pc1prob + pc2prob;
	prob += pcprob;
	*wp += pcprob; *wcnt += 1;
	sprintf(buf, "p(%s|%s_%s)=%.2f", wordIndex[cw], wordIndex[w], wordIndex[mw], pcprob);
	strcat(wbuf, buf); strcat(wbuf, " ");
	cc = ct = cw = 0; 
      }
      if( punc != 0 ){
	pc1prob = hashedComputeCP1Prob(pt, P, H, t, w, M, mt, mw, PUNC);
	pc2prob = hashedComputeCP2Prob(pw, pt, P, H, t, w, M, mt, mw, PUNC);
	pcprob = pc1prob + pc2prob;
	prob += pcprob;
	*wp += pcprob; *wcnt += 1;
	sprintf(buf, "p(%s|%s_%s)=%.2f", wordIndex[pw], wordIndex[w], wordIndex[mw], pcprob);
	strcat(wbuf, buf); strcat(wbuf, " ");	
	punc = pt = pw = 0;
      }

      d = OTHER_TAU;
      frameR = removeSubcat(t1->synT, frameR);     
    }
    if( frameR!=0 )
      { fprintf(stderr, "Warning: Right subcat non-empty: %d\n", frameR); exit(18); }
    
    if( !strcmp(tree->synT, "NPB") ){
      for(t1=tree->headchild; t1->siblingR!=NULL; t1=t1->siblingR) ;
      chead = t1;
      cvi = 0; ctau = START_TAU;
    }
    else{
      chead = tree->headchild;
      cvi = viStop; ctau = d;
    }

    getLex(lex, chead->lex, 0);
    H = getLabelIndex(chead->synT, 0); t = getLabelIndex(chead->preT, 0); 
    w = getWordIndex(lex, 0);
    prob += hashedComputeMod1Prob(0, 0, P, H, t, w, frameR, cvi, ctau, cc, punc, 2);

    tree->prob = prob + cprob;
    return tree->prob;
  }
  /* here? impossible! */
  fprintf(stderr, "Could not have reached this point\n");
  return 0;
}

