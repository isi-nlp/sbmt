#ifndef VECTOR_H
#define VECTOR_H

/*
vector.h
David Chiang <chiang@isi.edu>

Copyright (c) 2004-2006 University of Maryland. All rights
reserved. Do not redistribute without permission from the author. Not
for commercial use.
*/

typedef struct {
  void **x;
  int n, max;
  float grow_factor;
} vector_t;

vector_t *new_vector(int max);
void *vector_get(vector_t *v, int i);
int vector_set(vector_t *v, int i, void *y);
int vector_push(vector_t *v, void *y);
void **vector_freeze(vector_t *v, int *pn);
void vector_delete(vector_t *v);
int vector_clear(vector_t *v);

#endif
