#ifndef JMPHASH_H
#define JMPHASH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "key.h"

#define MAXJMP_S 250
#define MAXJUMPS 1000
/* it's looping (full?) if jumps more than that */

typedef struct {
  int count;
  Key key;
  unsigned char s;
} JmpNode;

typedef struct {
  int size;
  int total;
  JmpNode *table;
  unsigned char s;
  int max_jump;
} JmpHash;

/*assumes the following functions on Key:

  hashIndex(Key *key, int size) function from key -> value 0<=value<size
  equalKey(Key *k1, Key*k2) returns 1 if the keys are equal, 0 otherwise
  Key* createKey(Key *k2) creates an indentical key
*/

unsigned int hashFunction(Key *key, JmpHash *jmphash, int init, int start, int end);

void createJmpHash(int size, JmpHash *jmphash);
int findIndexJmpHash(Key *key, JmpHash *jmphash);
int addElementJmpHash(Key *key, JmpHash *jmphash, int count);
int setElementJmpHash(Key *key, JmpHash *jmphash, int count);
int findElementJmpHash(Key *key, JmpHash *jmphash);

void newsentJmpHash(JmpHash *jmphash);
void clearJmpHash(JmpHash *jmphash);

#endif
