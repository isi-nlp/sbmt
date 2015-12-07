/*
data.c
David Chiang <chiang@isi.edu>

Copyright (c) 2004-2006 University of Maryland. All rights
reserved. Do not redistribute without permission from the author. Not
for commercial use.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "point.h"
#include "vector.h"
#include "strstrsep.h"

extern int comps_n;

data_t *read_data(FILE *fp) {
  static char buf[1000];
  char *rest, *subrest, *tok, *subtok;
  int field, subfield;
  data_t *data;
  int sent_i, cand_i;
  vector_t *sents, *cands;
  candidate_t *cand;

  sents = new_vector(100);

  while (fgets(buf, sizeof(buf), fp) != NULL) {
    field = 0;
    rest = buf;
    while ((tok = strstrsep(&rest, "|||")) != NULL) {
      if (field == 0) {
	sent_i = strtol(tok, NULL, 10);
	if (sent_i < sents->n-1) {
	  fprintf(stderr, "sentence numbers should be nondecreasing\n");
	}
	while (sents->n-1 < sent_i)
	  vector_push(sents, new_vector(100));
	cand = malloc(sizeof(candidate_t));
	vector_push((vector_t *)vector_get(sents, sents->n-1), cand);
      }
      else if (field == 1) {
	cand->comps = malloc(comps_n*sizeof(int));
	subfield = 0;
	subrest = tok;
	while ((subtok = strsep(&subrest, " \t\n")) != NULL) {
	  if (!*subtok) // empty token
	    continue;
	  cand->comps[subfield] = strtol(subtok, NULL, 10);
	  subfield++;
	}
      }
      else if (field == 2) {
	cand->features = malloc(dim*sizeof(float));
	subfield = 0;
	subrest = tok;
	while ((subtok = strsep(&subrest, " \t\n")) != NULL) {
	  if (!*subtok) // empty token
	    continue;
	  cand->features[subfield] = strtod(subtok, NULL);
	  subfield++;
	}
      } else {
	fprintf(stderr, "too many fields in n-best list line\n");
      }
	
      field++;
    } 
  }

  // put the growable arrays into normal arrays
  data = malloc(sizeof(data_t));
  data->sents_n = sents->n;
  data->sents = malloc(sents->n * sizeof(candidate_t *));
  data->cands_n = malloc(sents->n * sizeof(int));
  for (sent_i = 0; sent_i < sents->n; sent_i++) {
    cands = vector_get(sents, sent_i);
    data->sents[sent_i] = malloc(cands->n * sizeof(candidate_t));
    data->cands_n[sent_i] = cands->n;
    for (cand_i = 0; cand_i < cands->n; cand_i++) {
      data->sents[sent_i][cand_i] = *(candidate_t *)vector_get(cands, cand_i); // struct copy
    }
    vector_delete(cands);
  }
  vector_delete(sents);

  return data;
}

