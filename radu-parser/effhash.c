#include <assert.h>

#include "effhash.h"

#define MAXEFFKEYLEN 50

void createEffHash(int size, EffHash *effhash)
{
  int i;

  if( (effhash->table=(EffNode *) malloc(size*sizeof(EffNode)))== NULL )
    { fprintf(stderr, "Cannot allocate memory for %d entries in new effhash table", size); exit(2); }

  effhash->total = 0;
  effhash->size=size;
  clearEffHash(effhash);
  for(i=0;i<size;i++){
      effhash->table[i].key.key = (unsigned char *) mymalloc_char(MAXEFFKEYLEN);
      effhash->table[i].key.len = 0;
  }
}

int findElementEffHash(Key *key, EffHash *effhash)
{
  int index;

  index = hashIndex(key, effhash->size);

  while( effhash->s==effhash->table[index].s && !equalKey(&(effhash->table[index].key),key) ){
    index++;
    if(index >= effhash->size)
      index = 0;
  }
  return index;
}

double findProbEffHash(Key *key, EffHash *effhash, int *flag)
{
  int pos;

  pos = findElementEffHash(key, effhash);

  if( effhash->table[pos].s != effhash->s )
    { *flag = 0; return 0; }

  *flag = 1;
  return effhash->table[pos].prob;
}

void addProbEffHash(Key *key, EffHash *effhash, double prob)
{
  int pos;

  pos = findElementEffHash(key, effhash);

  if( effhash->table[pos].s != effhash->s ){
    if( effhash->total < (effhash->size/2) ){
      effhash->table[pos].s = effhash->s;
      keyCopy2(&effhash->table[pos].key, key);
      effhash->table[pos].prob = prob;
      (effhash->total)++;
    }
  }
  
}

void clearEffHash(EffHash *effhash)
{
  int i;
  for(i=0;i<effhash->size;i++)
    effhash->table[i].s=0;
  effhash->s=1;
}

void newsentEffHash(EffHash *effhash)
{
  effhash->total = 0;
  if( effhash->s==MAXEFF_S )
    clearEffHash(effhash);
  else
    (effhash->s)++;
}
