/*
score.c
David Chiang <chiang@isi.edu>

Copyright (c) 2004-2006 University of Maryland. All rights
reserved. Do not redistribute without permission from the author. Not
for commercial use.
*/


#include <math.h>
#include <stdio.h>

#include "score.h"

int comps_n = 0;
int score_mode = 0;
#define MODE_BLEU 1
#define MODE_TER 2
#define MODE_WAVG 3

void init_bleu(int n) {
  comps_n = 2*n+1;
  score_mode = MODE_BLEU;
}

void init_ter(void) {
  comps_n = 2;
  score_mode = MODE_TER;
}

void init_wavg(void) {
  comps_n = 2;
  score_mode = MODE_WAVG;
}

void comps_addto(int *comps1, int *comps2) {
  int i;
  for (i=0; i<comps_n; i++)
    comps1[i] += comps2[i];
}

float compute_bleu(int *comps) {
  float logbleu = 0.0, brevity;
  int i;
  int n = (comps_n-1)/2;

  /*for (i=0; i<comps_n; i++)
    fprintf(stderr, " %d", comps[i]);
    fprintf(stderr, "\n");*/

  for (i=0; i<n; i++) {
    if (comps[2*i] == 0)
      return 0.0;
    logbleu += log(comps[2*i])-log(comps[2*i+1]);
  }
  logbleu /= n;
  brevity = 1.0-(float)comps[comps_n-1]/comps[1]; // comps[comps_n-1] is the ref length, comps[1] is the test length
  if (brevity < 0.0)
    logbleu += brevity;

  if (logbleu > 0.0) {
    fprintf(stderr, "BLEU better than 1, this shouldn't happen\n");
    for (i=0; i<comps_n; i++)
      fprintf(stderr, " %d", comps[i]);
    fprintf(stderr, "\n");
  }

  return exp(logbleu);
}

float compute_ter(int *comps) {
  return 1.0-((float)comps[0]/comps[1]);
}

float compute_wavg(int *comps) {
  return (float)comps[0]/comps[1];
}

float compute_score(int *comps) {
  switch (score_mode) {
  case MODE_BLEU: return compute_bleu(comps);
  case MODE_TER: return compute_ter(comps);
  case MODE_WAVG: return compute_wavg(comps);
  }
}

