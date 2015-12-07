/*
data.h
David Chiang <chiang@isi.edu>

Copyright (c) 2004-2006 University of Maryland. All rights
reserved. Do not redistribute without permission from the author. Not
for commercial use.
*/

#ifndef DATA_H
#define DATA_H

#include <stdio.h>

typedef struct {
  float *features;
  int *comps;
  float m, b; // slope and intercept, used as scratch space
} candidate_t;

typedef struct {
  candidate_t **sents;
  int sents_n, *cands_n;
} data_t;

data_t *read_data(FILE *fp);

#endif
