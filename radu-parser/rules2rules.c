#include "rules.h"

#define CONVERT2MT 1

int main(int argc, char **argv)
{
  Tree *tree;
  int ntree=0, flag;
  FILE *f;
  char buf[80];

  strcpy(MISC_SUBCAT, "MISC-C");
  strcpy(S_SUBCAT, "S-C");
  strcpy(START_TAU, "+START+");

  strcpy(NAS, "+NAs+");
  strcpy(NAT, "+NAt+");
  strcpy(NAW, "+NAw+");

  if( (f=fopen(argv[1], "r"))==NULL)
    { fprintf(stderr, "Cannot open file %s\n", argv[1]); exit(2); }

  fscanf(f, "%s", buf); 
  assert(!strcmp(buf, "+hf+"));
  flag = 1;

  while( flag ){
    tree = rules2tree(f, 0, &flag); 
    if( CONVERT2MT )
      convertPTBTree2MTTree(tree);

    updateCV(tree);
    updateVi(tree);
    updateTreeText(tree);

    extractPlainRules(tree, ntree);
      
    ntree++;
  }
  fclose(f);
}


