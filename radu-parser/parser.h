#ifndef PARSER_H
#define PARSER_H

#include <assert.h>
#include <sys/time.h>
#include "model2.h"
#include "effhash.h"
#include "tree.h"
#include "sentence.h"
#include "prob.h"

#define LR 0
#define SHOWSTEPS 0

#define MAXEDGES     900000
#define MAXCHILDREN 5000000
#define MAXNHEAD      10000
#define MAXNMOD       10000


#define MAXUNARYREWRITES 5

#define MINLEAFPROB -500

double BEAM;
int DYNBEAM, GBEAM, DYNGBEAM;
int NBEST, NBEAM;
double BEAMS[MAXNTS][MAXWORDS], GBEAMS[MAXNTS][MAXWORDS][MAXWORDS];

double bestGThreshProb;
int gbeamthresh, maxspan4gbeam;

int mttokFlag;

typedef struct{
  int tau;
  int vi;
  int subcat; /*   np s sbar vp other */
} Context;

typedef struct struct_edge{
  int type;
  char *label, *headlabel, *headtag, *headword;
  int labelI, headlabelI, headtagI, headwordI;
  int stop;
  Context lc, rc;
  int start, end;
  double prob, prior, prob2; /* prob 2 is prob + prior */
  int child1;         /* index in children[]: from child1 to nchildren-1 */
  int nchildren;
  int brick1, brick2; /* index in edges[]: the edges (bricks) that support current edge */
  int next;           /* next in sNT[] */
  int headindex;
  int cv;    /* contains verb */
  int inbeam; /* 0/1 ; 2 when not checked yet */
} Edge;

/*
   edges is an array of edges in the chart

   childs is an array used for the children of each node in the chart:

   children[edge[i].child1] to children[edge[i].child1+edge[i].nchildren-1] are
   the children of the i'th edge in left to right order (being indexes into
   the edges array themselves)
 */
Edge edges[MAXEDGES];
int nedges;
int ntheories;

int children[MAXCHILDREN];
int nchildren;

/*
   The edges in the chart spanning words s to e inclusive are contained
   in edges[sindex[s][e]] to edges[eindex[s][e]] inclusive

   if no edges have been added to the chart for the span s..e,
   sindex[s][e]==-1, eindex[s][e]==-2
 */
int sindex[MAXWORDS][MAXWORDS];
int eindex[MAXWORDS][MAXWORDS];

/*
   SNT is a data structure that supports add_edge: allows edges in the chart with 
   the same non-terminal label to be stored in a linked list, by pointing to the 
   first element of the linked list. 
   This helps efficiency of the dynamic programming algorithm (when
   checking to see if there's an edge in the chart with the same label,
   head word etc. it's sufficient to look down the linked list for that
   non-terminal)

   saments contains information for non-terminals in the current span being
   worked on in the chart: at each point when a new span is started saments
   is re-initialised 

   saments[i] contains the following info for non-terminal i:

     edge1 is the first edge in the chart with label i. edges[i].next points
     to the next element in the linked list, and so on
     numedges is the number of edges with the i'th non-terminal
*/

typedef struct {
  int edge1;
  char nedges;
} SNT;

SNT sNT[MAXNTS];

double maxprob[MAXNTS][MAXNTS];
int   maxIndex[MAXNTS][MAXNTS];

double forward[MAXWORDS], backward[MAXWORDS];
int bp[MAXWORDS], bplabelI[MAXWORDS];

int cpos, corigpos;

extern int START_TAU, OTHER_TAU;
extern int rmC[MAXNTS], unk[MAXVOC];
extern int labelIndex_NP, labelIndex_NPB, labelIndex_PP, labelIndex_CC, labelIndex_Comma, labelIndex_Colon, labelIndex_LRB, labelIndex_RRB, labelIndex_S1;
extern int labelIndex_NPC, labelIndex_SC, labelIndex_SGC, labelIndex_SBARC, labelIndex_VPC;
 
EffHash effhashProb;

/*
  +h+ -> 1

  +lf+ -> 11
  +rf+ -> 12

  +lm1+ -> 21
  +rm1+ -> 22

  +lm2+ -> 31
  +rm2+ -> 32

  +p1+ -> 41
  +c1+ -> 42

  +p2+ -> 51
  +c2+ -> 52

  +prior+ -> 61
 */

extern JmpHash jmphashCnt;
extern char *wordTag[MAXVOC][MAXTAGS];
extern int nwordTag, cnwordTag[MAXVOC];
extern int wordTagI[MAXVOC][MAXTAGS], totalWordCnt;
extern char *labelIndex[MAXNTS], *wordIndex[MAXVOC];
extern int nlabelIndex;

extern int lframe[MAXNTS][MAXNTS][MAXSUBCATS], nlframe[MAXNTS][MAXNTS];
extern int rframe[MAXNTS][MAXNTS][MAXSUBCATS], nrframe[MAXNTS][MAXNTS];

int adds1[100000];
int nadds1;
int adds2[100000];
int nadds2;

int *adds;
int *nadds;
int ADDFLAG;

struct timeval mytime;
double parseTime, parseTimeS, parseTimeE;

int allb;

double parse(int cnt);
int initSent();
int addEdge(int s, int e);
int equalEdge(Edge *e1, Edge *e2);
void addSinglesStops(int start, int end);
void addSingles(int s, int e, int si, int ei);
void addStops(int s, int e, int si, int ei);
void complete(int s, int e);
int join2EdgesFollow(int hi, int s, int m, int e, int *e2s, int ne2s);
int join2EdgesCC(int hi, int mi, int s, int m, int e);
int join2EdgesPrecede(int hi, int s, int m, int e, int *e1s, int ne1s);
int inbeam(int edge, int s, int e);
int inbeam2(int edge, int s, int e);
void initChart();
void initSNT();

void writeEdgeTree(FILE *f, Edge *pe, Edge *e, int indent, int tab, int prob, int oneline);
void writeEdges(FILE *f, Edge *p, Edge *e, int labelIndex);
void writeEdgeLeaves(FILE *f, Edge *p, Edge *e);
void writeBeamInfo(FILE *f, int p);

void writeMTStyle(FILE *f, Edge *edge, int prob, int oneline);
void writeMTLex(FILE *f, char *olex, int start, int end);

void trainBeam(FILE *f);
void trainGBeam(FILE *f);

void initGThresh(int n);
void globalThreshold(int span, int n);
void computeFB(int span, int n, char cov[]);
void getMaxCoverage(char cov[], int bp[], int bplabelI[], int n);
int ingbeam(int i, int s, int e, int n);
void writeGBeamInfo(FILE *f, int p, int parentp, int n);
int extractEdgeCoverage(char maxcov[], int s, int e, int n, char edgecov[]);

#endif
