#ifndef EFFHASH_H
#define EFFHASH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "key.h"

#define MAXEFF_S 250

typedef struct {
  double prob;
  Key key;
  unsigned char s;
} EffNode;

typedef struct {
  int size;
  int total;
  EffNode *table;
  unsigned char s;
} EffHash;

/*assumes the following functions on Key:

  hashIndex(Key *key, int size) function from key -> value 0<=value<size
  equalKey(Key *k1, Key*k2) returns 1 if the keys are equal, 0 otherwise
  Key* createKey(Key *k2) creates an indentical key
*/

void createEffHash(int size, EffHash *effhash);
void addProbEffHash(Key *key, EffHash *effhash, double count);
int findElementEffHash(Key *key, EffHash *effhash);
double findProbEffHash(Key *key, EffHash *effhash, int *flag);

void newsentEffHash(EffHash *effhash);
void clearEffHash(EffHash *effhash);

#endif
