/*
point.c
David Chiang <chiang@isi.edu>

Copyright (c) 2004-2006 University of Maryland. All rights
reserved. Do not redistribute without permission from the author. Not
for commercial use.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "point.h"
#include "vector.h"

int dim = -1;
extern int fixed_dim;

point_t *new_point() {
  point_t *point;
  if (dim < 0) {
    fprintf(stderr, "new_point(): dim unknown\n");
    return NULL;
  }
  point = malloc(sizeof(point_t));
  point->score = 0.0;
  point->weights = calloc(dim, sizeof(float));
  point->has_score = 0;
  return point;
}

void point_set_score(point_t *point, float score) {
  point->has_score = 1;
  point->score = score;
}

void point_delete(point_t *point) {
  free(point->weights);
  free(point);
}

point_t *random_point(point_t *min, point_t *max) {
  int i;
  point_t *point = new_point();
  for (i=0; i<dim; i++)
    point->weights[i] = min->weights[i] + (float)random()/RAND_MAX * (max->weights[i]-min->weights[i]);
  return point;
}

point_t *point_copy(point_t *point) {
  point_t *newpoint;
  int i;
  newpoint = new_point();
  newpoint->score = point->score;
  newpoint->has_score = point->has_score;
  for (i=0; i<dim; i++)
    newpoint->weights[i] = point->weights[i];
  return newpoint;
}

float point_dotproduct(point_t *point, float *y) {
  float result;
  int i;
  result = 0.0;
  for (i=0; i<dim; i++)
    result += point->weights[i] * y[i];
  return result;
}

/* Destructive operations */
void point_multiplyby(point_t *point, float k) {
  int i;
  for (i=0; i<dim; i++)
    point->weights[i] *= k;
}

void point_addto(point_t *point1, point_t *point2) {
  int i;
  for (i=0; i<dim; i++)
    point1->weights[i] += point2->weights[i];
}

void point_normalize(point_t *point) {
  int i;
  float norm = 0.0, num, denom;

  if (fixed_dim >= 0 && fixed_dim < dim) {
    norm = fabs(point->weights[fixed_dim]);
  } else {

    for (i=0; i<dim; i++)
      //norm += point->weights[i] * point->weights[i];
      norm += fabs(point->weights[i]);
    // norm = sqrt(norm);
    
    // if any of them really wants to go nuts, just let it go nuts
    // without squashing the others
    num = norm; 
    denom = 1.0;
    for (i=0; i<dim; i++)
      if (fabs(point->weights[i]) > 2*norm/dim) {
	num -= fabs(point->weights[i]);
	denom -= 1.0/dim;
      }
    norm = num/denom;
  }

  for (i=0; i<dim; i++)
    point->weights[i] /= norm;
}

void point_print(point_t *point, FILE *fp, int with_score) {
  int i;
  fprintf(fp, "%f", point->weights[0]);
  for (i=1; i<dim; i++)
    fprintf(fp, " %f", point->weights[i]);
  if (point->has_score && with_score)
    fprintf(fp, " ||| %f", point->score);
}

// just used by read_point in the case where dim is unknown.
// caller owns both the vector and the elements.
vector_t *read_float_vector(FILE *fp) {
  static char buf[1000];
  char *tok, *s;
  vector_t *v;
  float *xp;

  v = new_vector(10);

  if (fgets(buf, sizeof(buf), fp) == NULL)
    return NULL;
  s = buf;
  while ((tok = strsep(&s, " \t\n")) != NULL) {
    if (!*tok) // empty token
      continue;
    xp = malloc(sizeof(float));
    *xp = strtod(tok, NULL);
    vector_push(v, xp);
  }
  if (v->n == 0) {
    vector_delete(v);
    return NULL;
  }
  return v;
}

point_t *read_point(FILE *fp) {
  static char buf[1000];
  char *tok, *s;
  int field;
  point_t *point;
  vector_t *v;
  float *xp;

  if (dim < 0) {
    v = read_float_vector(fp);
    if (v == NULL)
      return NULL;
    dim = v->n;
    point = new_point();
    for (field = 0; field<dim; field++) {
      xp = vector_get(v, field);
      point->weights[field] = *xp;
      free(xp);
    }
    vector_delete(v);
    return point;
  }

  point = new_point();

  if (fgets(buf, sizeof(buf), fp) == NULL)
    return NULL;
  s = buf;
  field = 0;
  while ((tok = strsep(&s, " \t\n")) != NULL) {
    if (!*tok) // empty token
      continue;
    if (field >= dim) {
      fprintf(stderr, "read_point(): too many fields in line\n");
      return NULL;
    } else
      point->weights[field] = strtod(tok, NULL);
    field++;
  }
  if (field == 0)
    return NULL;
  if (field < dim) {
    fprintf(stderr, "read_point(): wrong number of fields in line\n");
    return NULL;
  }
  return point;
}

point_t **read_points(FILE *fp, int *pn) {
  vector_t *v;
  point_t *p;
  v = new_vector(10);
  while ((p = read_point(fp)) != NULL)
    vector_push(v, p);
  return vector_freeze(v, pn);
}
