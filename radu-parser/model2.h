#ifndef MODEL2_H_inc
#define MODEL2_H_inc

#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "hash.h"
#include "jmphash.h"
#include "effhash.h"
#include "grammar.h"
#include "prob.h"

#define  HF_THRESHOLD 1
#define MOD_THRESHOLD 1

#define MAXMODS 20

#define SHOWHASHDENSITY 0

extern struct rusage *usage;

extern int wVOC; /* 1 means restricting loading lexical events to only test vocabulary-based */
extern int PPS;  /* 1 means pruned-per-sentence */ 
extern int SBLM; /* 1 means LM-like treatment of unk */
extern int IRENEHACK; /* 1 means dividing by NUNK (number of unknowns) the probs for events generating the UNK token */

extern FILE *vocSBLM;

extern int Debug; /* shows all internal values for probabilities */

extern int smoothLevel; /* 4 means last back-off is uniform distribution
		    5 means last back-offs are prior distribution on event and uniform distribution */

extern int THCNT;       /* ignores the flag from grammar.lex set by ThCnt; sets unk[] according to current THCNT */

extern double epsilonP; /* 0.0000000000000000001 = 10^(-19) */
extern int NNTS, NPTS, NFRS, NLEX, NUNK, tagNUNK[MAXNTS];

extern int STOPNT, CC, PUNC, HEAD, FRAME, MOD1, MOD2, LBL, TW, CP1, CP2, NAV;
extern int UNKINDEX, FFACTOR, FFACTOR_M2, FTERM;

extern int START_TAU, OTHER_TAU;
extern int rmC[MAXNTS], unk[MAXVOC], isnt[MAXNTS];
extern int labelIndex_NP, labelIndex_NPB, labelIndex_PP, labelIndex_CC, labelIndex_Comma, labelIndex_Colon, labelIndex_S1;
extern int labelIndex_NPC, labelIndex_SC, labelIndex_SGC, labelIndex_SBARC, labelIndex_VPC, labelIndex_NN;

extern int unaryI[MAXNTS][MAXNTS];

/*
extern   char *labelIndex[MAXNTS], *wordIndex[MAXVOC];
extern int nlabelIndex, nwordIndex;
extern int nunary[MAXNTS];
extern char *unaryL[MAXNTS][MAXNTS];
extern int lframe[MAXNTS][MAXNTS][MAXSUBCATS], nlframe[MAXNTS][MAXNTS];
extern int rframe[MAXNTS][MAXNTS][MAXSUBCATS], nrframe[MAXNTS][MAXNTS];

extern char *wordTag[MAXVOC][MAXTAGS];
extern int nwordTag, cnwordTag[MAXVOC];
extern int wordTagI[MAXVOC][MAXTAGS], totalWordCnt;

*/

extern int tablep[MAXNTS][MAXNTS][MAXNTS], tablef[MAXNTS][MAXNTS][MAXNTS];

extern Hash hashCnt;      /* for nts and lex */
extern JmpHash jmphashCnt;/* for events */
extern EffHash effhashProb;/* for probs */


/*
Hash indexes:

"headJointCnt_1", "headCondCnt_1", "headOutcomeCnt_1" 1-3  1->9
"frameJointCnt_1", "frameCondCnt_1" 1-3                    11->19
"mod1JointCnt_1", "mod1CondCnt_1", "mod1OutcomeCnt_1" 1-3  21->29
"mod2JointCnt_1", "mod2CondCnt_1", "mod2OutcomeCnt_1" 1-3  31->39
"cp1JointCnt_1", "cp1CondCnt_1", "cp1OutcomeCnt_1" 1-3     41->49 
"cp2JointCnt_1", "cp2CondCnt_1", "cp2OutcomeCnt_1" 1-3     51->59 

"topNTJointCnt", "topNTCondCnt"                            61->62        
"topWJointCnt", "topWCondCnt", "topWOutcomeCnt"            64->66

"prior1JointCnt_1", "prior1CondCnt_1", "prior1OutcomeCnt_1" 1-3 71->79
"prior2JointCnt", "prior2CondCnt"                               81->82

"synT" 91 
"preT" 92
"lex" 93
"lexIndex" 94
"lexPreT" 95

label-as-key 101
word-as-key  102
voc-as-key   103
*/

#include "sblm.h"

#endif
