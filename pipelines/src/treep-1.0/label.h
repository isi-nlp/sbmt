/* Copyright (c) 2002 by David Chiang. All rights reserved.*/

#ifndef LABEL_H
#define LABEL_H

#include <stdlib.h>

typedef struct {
  char *base, **tags, *delimiters;
  int n_tags, max_tags;
} *label;

void label_delete(label l);

int label_match(label l, const char *s);
int label_set(label l, const char *s);
int label_reset(label l, const char *s);

int label_to_string(label l, char *s, size_t n);
label label_from_string(const char *s);

#endif

