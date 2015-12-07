#ifndef RULES_H
#define RULES_H

#include "tree.h"

#define MODEL3L 0

#define SLEN 80

char MISC_SUBCAT[SLEN], S_SUBCAT[SLEN], START_TAU[SLEN];
int allb;

char NAS[SLEN], NAT[SLEN], NAW[SLEN];

Tree* rules2tree(FILE *f, Tree *parent, int *flag);
int readRuleMod(FILE *f, Tree *parent, Tree *head, Tree *lastSibling, char *dir);

void extractPlainRules(Tree *tree, int cnt);

char* removeSubcat4S(char *label, char *subcat);
char* mapSubcat2S(char *synT);

void quicksortStrings(char* R[], int p, int r, int N);
int partitionStrings(char* R[], int p, int r);
void swapStrings(char **r1, char **r2);

#endif
