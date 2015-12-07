#ifndef KEY_H
#define KEY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mymalloc.h"
#include "mymalloc_char.h"

typedef struct {
  unsigned char *key;
  int len;
} Key;

extern int strChunks, mallocChunksKey;

int hashIndex(Key *key, int size);
int equalKey(Key *k1, Key *k2);
void keyCopy(Key *k1, Key *k2);
void keyCopy2(Key *k1, Key *k2);

#endif
