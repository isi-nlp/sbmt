#include "tree.h"

int allb;

int rules2trees(char *fname, Tree *tree[]);
Tree* rules2tree(FILE *f, Tree *parent, int *flag);
int readMod(FILE *f, Tree *parent, Tree *head, Tree *lastSibling, char *dir);


int main(int argc, char **argv)
{
  Tree *tree[100000];
  int ntree;

  ntree = rules2trees(argv[1], tree);
  fprintf(stderr, "Trees read & written: %d\n", ntree);
}

int rules2trees(char *fname, Tree *tree[])
{
  FILE *f;
  int ntree=0, flag;
  char buf[80];

  if( (f=fopen(fname, "r"))==NULL)
    { fprintf(stderr, "Cannot open file %s\n", fname); exit(2); }
  fscanf(f, "%s", buf); 
  assert(!strcmp(buf, "+hf+"));
  flag = 1;
  while( flag ){
    tree[ntree] = rules2tree(f, 0, &flag); 
    fprintf(stderr, "Writing tree #%d\n", ntree);
    writeTree(tree[ntree], 0, 0, 1);
    ntree++;
  }
  close(f);
  return ntree;
}

