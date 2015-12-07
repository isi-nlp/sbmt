#ifndef SENTENCE_H
#define SENTENCE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXSENT        5001
#define MAXWORDS         90
#define MAXCHARS         50

typedef struct{
  int nwords;
  char word[MAXWORDS][MAXCHARS], tag[MAXWORDS][MAXCHARS];
  int wordI[MAXWORDS], tagI[MAXWORDS];
  int hasPunc[MAXWORDS], hasPunc4Prune[MAXWORDS];
  char wordPunc[MAXWORDS][MAXCHARS], tagPunc[MAXWORDS][MAXCHARS];
  int wordIPunc[MAXWORDS], tagIPunc[MAXWORDS];
} Sent;

Sent sent[MAXSENT], origsent[MAXSENT], *csent, *corigsent;
int nsent; 

extern int labelIndex_Comma, labelIndex_Colon;
int labelIndex_LRB, labelIndex_RRB;

void readSentences(char *fname);
void writeFlatTree(FILE *f);

#endif
