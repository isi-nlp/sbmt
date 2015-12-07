#include <stdlib.h>

/*
vector.c
David Chiang <chiang@isi.edu>

Copyright (c) 2004-2006 University of Maryland. All rights
reserved. Do not redistribute without permission from the author. Not
for commercial use.
*/

#include "vector.h"

vector_t *new_vector(int max) {
  vector_t *v = malloc(sizeof(vector_t));
  v->n = 0;
  v->max = max;
  v->x = malloc(max*sizeof(void *));
  v->grow_factor = 1.5;
  return v;
}

void *vector_get(vector_t *v, int i) {
  if (i < 0 || i > v->max)
    return NULL;
  return v->x[i];
}

int vector_set(vector_t *v, int i, void *y) {
  if (i < 0 || i > v->max)
    return -1;
  v->x[i] = y;
  return 0;
}

int vector_push(vector_t *v, void *y) {
  v->n++;
  if (v->n > v->max) {
    v->max = (v->max+1) * v->grow_factor;
    v->x = realloc(v->x, v->max*sizeof(void *));
  }
  v->x[v->n-1] = y;
  return 0;
}

int vector_clear(vector_t *v) {
  v->n = 0;
  return 0;
}

void **vector_freeze(vector_t *v, int *pn) {
  void *x;
  x = realloc(v->x, v->n*sizeof(void *));
  if (pn != NULL)
    *pn = v->n;
  free(v);
  return x;
}

void vector_delete(vector_t *v) {
  free(v->x);
  free(v);
}
