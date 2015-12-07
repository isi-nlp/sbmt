#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "hash.h"
#include "jmphash.h"

#include "sblm_limits.h"

extern int STOPNT, UNKINDEX, NAV;
extern char RulesTrain[];

extern char *labelIndex[MAXNTS], *wordIndex[MAXVOC];
extern int nlabelIndex, nwordIndex;
extern int unaryC[MAXNTS][MAXNTS], nunary[MAXNTS];
extern char *unaryL[MAXNTS][MAXNTS];
extern int lframe[MAXNTS][MAXNTS][MAXSUBCATS], lframeC[MAXNTS][MAXNTS][MAXSUBCATS], nlframe[MAXNTS][MAXNTS];
extern int rframe[MAXNTS][MAXNTS][MAXSUBCATS], rframeC[MAXNTS][MAXNTS][MAXSUBCATS], nrframe[MAXNTS][MAXNTS];

extern char *wordTag[MAXVOC][MAXTAGS];
extern int nwordTag, wordTagC[MAXVOC][MAXTAGS], cnwordTag[MAXVOC];
extern int wordTagI[MAXVOC][MAXTAGS], totalWordCnt;

extern JmpHash jmphashCnt;
extern Hash hashCnt;

void grammar(char *headframefile, char *modfile, char *Grammar);
void extractNodeInfo(int L, int cnt);
void extractLeafInfo(char *preT, int t, int w, int cnt, int recordFlag);
int getLabelIndex(char *label, int addflag);
int getWordIndex(char *word, int addflag);
int getVocIndex(char *word, int addflag);
void updateFrameList(char *parent, int p, char *head, int h, int frameL, int frameR);
int encodeFrame(char *frame);
int mapSubcat(char *label);

int writeWord(unsigned char* string, int n, int i);
int writeFrame(unsigned char* string, int n, int i);
int write2Bytes(unsigned char* string, int n, int i);
int write3Bytes(unsigned char* string, int n, int i);

void insertSortLex(char *s, char *list[], int listC[], int *nlist);
void insertSortInt(int s, int list[], int listC[], int *nlist);
int lessLex(char *r1, char *r2);

#endif
