#include <assert.h>

#include "jmphash.h"

#define MAXJMPKEYLEN 50

void createJmpHash(int size, JmpHash *jmphash)
{
  int i;

  if( (jmphash->table=(JmpNode *) malloc(size*sizeof(JmpNode)))== NULL )
    { fprintf(stderr, "Cannot allocate memory for %d entries in new jmphash table", size); exit(2); }

  jmphash->total = 0;
  jmphash->size=size;
  jmphash->s = 1;
  jmphash->max_jump = 0;
  clearJmpHash(jmphash);
  for(i=0;i<size;i++){
      jmphash->table[i].key.key = (unsigned char *) mymalloc_char(MAXJMPKEYLEN);
      jmphash->table[i].key.len = 0;
      jmphash->table[i].count = 0;
  }
}

unsigned int hashFunction1(Key *key, JmpHash *jmphash, int init, int start, int end)
{
  unsigned int hash = 5381;
  int i; 
  
  if( init ) hash = ((hash << 5) + hash) + init;

  for(i=start; i<end; i++)
    hash = ((hash << 5) + hash) + key->key[i]; // hash*33 + c
  return hash % jmphash->size;
}

unsigned int hashFunction2(Key *key, JmpHash *jmphash, int init, int start, int end)
{
  unsigned int hash = init;
  int i;
  
  for(i=start; i<end; i++)
    hash = key->key[i] + (hash << 6) + (hash << 16) - hash;
  return hash % jmphash->size;
}


int findIndexJmpHash(Key *key, JmpHash *jmphash)
{
  int cnt;
  unsigned int index, index0, index1, index2;

  index1 = hashFunction1(key, jmphash, 0, 0, key->len);
  index = index1;
  cnt = 0;
  while( jmphash->s==jmphash->table[index].s ){
    if( !equalKey(&(jmphash->table[index].key),key) ){
      index2 = hashFunction2(key, jmphash, index, 0, key->len);
      index0 = 2 << cnt;
      if( index0 > index2 )
	{ index++; if( index==jmphash->size) index = 0; }
      else index = (index + index2) % jmphash->size;
    }
    else
      return index;

    if( cnt++>MAXJUMPS )
      if( jmphash->total/(double)jmphash->size>0.90 )
	fprintf(stderr, "Warning: jumping the %dth time (total in jmphash %d): index = %d\n", cnt, jmphash->total, index);
    if( jmphash->total/(double)jmphash->size>0.95 )
      { fprintf(stderr, "Error: hash for training events full more than 95 percent\n"); exit(22); }
    if( jmphash->max_jump < cnt ) jmphash->max_jump = cnt;
  }
  return index;
}

int addElementJmpHash(Key *key, JmpHash *jmphash, int count)
{
  unsigned int pos;
  
  pos = findIndexJmpHash(key, jmphash);
  
  if( jmphash->table[pos].s != jmphash->s )
    (jmphash->total)++;
  jmphash->table[pos].s = jmphash->s;
  keyCopy2(&jmphash->table[pos].key, key);
  jmphash->table[pos].count += count;
  return jmphash->table[pos].count;
}

int setElementJmpHash(Key *key, JmpHash *jmphash, int count)
{
  unsigned int pos;
  
  if( count==0 ) return 0; /* count==0 is for events involving +N/A+ (oov items) */

  pos = findIndexJmpHash(key, jmphash);
  
  if( jmphash->table[pos].s != jmphash->s ){
    (jmphash->total)++;
    jmphash->table[pos].s = jmphash->s;
    keyCopy2(&jmphash->table[pos].key, key);
    jmphash->table[pos].count = count;
  }
  else{
    if( jmphash->table[pos].count != count )
      fprintf(stderr, "Warning: the same element found with different values: %d vs. %d\n", jmphash->table[pos].count, count);
  }
  return count;
}

int findElementJmpHash(Key *key, JmpHash *jmphash)
{
  unsigned int pos;
  
  pos = findIndexJmpHash(key, jmphash);

  if( jmphash->s==jmphash->table[pos].s )
    return jmphash->table[pos].count;
  return 0;
}

void clearJmpHash(JmpHash *jmphash)
{
  int i;
  for(i=0;i<jmphash->size;i++)
    jmphash->table[i].s=0;
  jmphash->s=1;
}

void newsentJmpHash(JmpHash *jmphash)
{
  jmphash->total = 0;
  jmphash->max_jump = 0;
  if( jmphash->s==MAXJMP_S )
    clearJmpHash(jmphash);
  else
    (jmphash->s)++;
}

int countByStamp(JmpHash *jmphash, int s)
{
  int i, cnt=0;
  for(i=0; i<jmphash->size; i++)
    if( jmphash->table[i].s==s )
      cnt++;
  return cnt;
}
