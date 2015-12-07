#include <assert.h>
#include "key.h"

int hashIndex(Key *key,int size)
{
  int i;
  unsigned int index;

  index = key->key[0];
  for(i=1;i<key->len;i++)
    index = (index*256 + key->key[i]) % size;

  return index;
}


int equalKey(Key *k1, Key *k2)
{
  int i;

  if(k1->len!=k2->len)
    return 0;

  for(i=0;i<k1->len;i++)
    if(k1->key[i] != k2->key[i])
      return 0;
  
  return 1;
}

void keyCopy(Key *k1, Key *k2)
{
  int i;

  k1->len = k2->len;

  k1->key = (unsigned char *) mymalloc_char(k2->len);

  for(i=0;i<k1->len;i++)
    k1->key[i] = k2->key[i];
}

void keyCopy2(Key *k1, Key *k2)
{
  int i;

  k1->len = k2->len;

  for(i=0;i<k1->len;i++)
    k1->key[i] = k2->key[i];
}




