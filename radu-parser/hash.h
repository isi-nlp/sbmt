#ifndef HASH_H
#define HASH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "key.h"

typedef struct hash_node {
  struct hash_node *next;
  int count;
  Key key;
} Node;

typedef struct {
  int size;
  int total;
  Node **table;
} Hash;

/*assumes the following functions on Key:

  hashIndex(Key *key, int size) function from key -> value 0<=value<size
  equalKey(Key *k1, Key*k2) returns 1 if the keys are equal, 0 otherwise
  Key* createKey(Key *k2) creates an indentical key
*/

/* creates a hash table with size elements*/
void createHash(int size, Hash *hash);

/*creates new element if key is not already in the hash table, otherwise increments count
  returns count for key
*/
int addElementHash(Key *key, Hash *hash, int count);
/* same as above, it only sets the count */
int setElementHash(Key *key, Hash *hash, int count);

/*returns the count for key in hash table (0 if not there)*/
int findElementHash(Key *key, Hash *hash);

#endif
