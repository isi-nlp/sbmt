#include <assert.h>

#include "hash.h"

void createHash(int size, Hash *hash)
{
  int i;

  if( (hash->table=(Node **) malloc(size*sizeof(Node*)))== NULL )
    { fprintf(stderr, "Cannot allocate memory for %d entries in new hash table", size); exit(2); }

  hash->total = 0;
  for(i=0;i<size;i++)
    hash->table[i]=NULL;
  hash->size=size;
}

int addElementHash(Key *key, Hash *hash, int count)
{
  int index;
  Node *h,*p=NULL;

  index = hashIndex(key, hash->size);

  if( hash->table[index]==NULL )
    {
      if( (h = (Node*) mymalloc(sizeof(Node)))==NULL )
	{ fprintf(stderr, "Cannot allocate memory for new entry in hash table"); exit(2); }

      hash->table[index] = h;
      hash->total++;
      keyCopy(&(h->key),key);
      h->count = count;
      h->next = NULL;
      return count;
    }    

  h = hash->table[index];
  //assert(h);
  while( h!=NULL && !equalKey(&h->key, key))
    {
      p = h;
      h = h->next;
    }
  if(h == NULL)
    {
      if( (h = (Node*) mymalloc(sizeof(Node)))==NULL )
	{ fprintf(stderr, "Cannot allocate memory for new entry in hash table"); exit(2); }
      hash->total++;
      //assert(p);
      p->next = h;
      keyCopy(&(h->key),key);
      h->count = count;
      h->next = NULL;
      return count;
    }

  h->count += count;
  return h->count;
}

int setElementHash(Key *key, Hash *hash, int count)
{
  int index;
  Node *h,*p=NULL;

  index = hashIndex(key, hash->size);

  if( hash->table[index]==NULL )
    {
      if( (h = (Node*) mymalloc(sizeof(Node)))==NULL )
	{ fprintf(stderr, "Cannot allocate memory for new entry in hash table"); exit(2); }

      hash->table[index] = h;
      hash->total++;
      keyCopy(&(h->key),key);
      h->count = count;
      h->next = NULL;
      return count;
    }    

  h = hash->table[index];
  //assert(h);
  while( h!=NULL && !equalKey(&h->key, key))
    {
      p = h;
      h = h->next;
    }
  if(h == NULL)
    {
      if( (h = (Node*) mymalloc(sizeof(Node)))==NULL )
	{ fprintf(stderr, "Cannot allocate memory for new entry in hash table"); exit(2); }
      hash->total++;
      //assert(p);
      p->next = h;
      keyCopy(&(h->key),key);
      h->count = count;
      h->next = NULL;
      return count;
    }

  h->count = count;
  return h->count;
}

int findElementHash(Key *key, Hash *hash)
{
  int index;
  Node *h;

  index = hashIndex(key, hash->size);

  h = hash->table[index];

  while( h!=NULL && !equalKey(&h->key,key) )
    h = h->next;

  if( h==NULL )
    return 0;

  return h->count;
}

