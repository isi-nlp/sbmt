#ifndef TREE_H
#define TREE_H

#include <stdio.h>
#include <string.h>
#include <assert.h>

#define MAXPIECE 30
#define SLEN     80
#define MAXMODS  20

typedef struct struct_Tree{
  char synT[SLEN]; /* this is synactic tag */
  char semT[SLEN]; /* this is semantic tag */
  char lex[SLEN];
  char preT[SLEN]; /* this is preTerminal, i.e. POS */
  int cv;     /* this is contains-verb predicate */
  int lvi, rvi; /* this is left and right-verb intervening - Collins style */
  double prob;
  int ntoken; /* number of tokens in the tree; token = nodes + leaves */
  int b, e;    /* begining/end of spanned text as words */
  struct struct_Tree* headchild; /* head child */
  struct struct_Tree* child;     /* left-most child */
  struct struct_Tree *siblingL, *siblingR;
  struct struct_Tree *parent;
} Tree;

Tree* readTree(FILE *f, int preprocessFlag);
Tree* readTree1(FILE *f, Tree *parent, Tree *siblingL, int *off, char last[]);
int increaseLeavesCount(char *pos, char *lex, char last[]);

Tree* readCTree(FILE *f);
Tree* readCTree1(FILE *f, Tree *parent, int *off);
Tree* readCNode(FILE *f, int *hidx);
Tree* readCLeaf(FILE *f, Tree *parent, char c, int *off);
void updatePreT(Tree *tree);

Tree* readFullTree(FILE *f, int mttokFlag, int *bug);
Tree* readFullTree1(FILE *f, Tree *parent, Tree *siblingL, int *off, char last[], int mttokFlag, int *bug);

void convertPTBTree2MTTree(Tree *tree);
void convertPTBLex2MTLex(Tree *tree);
void convertPTBLex2ComplexNT(Tree *tree, int b, int e, char piece[][80], int npiece);

void updatePruning(Tree *tree);
int updateNPBs(Tree *tree);
void updateRepairNPs(Tree *tree, char *label);
void updateSpans(Tree *tree);
void updateLexHead(Tree *tree);
void updateTrace(Tree *tree);
void updateProS(Tree *tree);
void updateRemoveNull(Tree *tree);
void updatePunct(Tree *tree);
void updateComplements(Tree *tree);
void updateRepairProS(Tree *tree);

int updateNToken(Tree *tree);
void updateCV(Tree *tree);
void updateVi(Tree *tree);

void postprocessTree(Tree *tree);
void undoComplements(Tree *tree);
void undoPunct(Tree *tree);
void undoProS(Tree *tree);
void undoRepairNPBs(Tree *tree);
void undoNPBs(Tree *tree);

int lexHead(Tree *tree, char lex[], char pret[]);
int  lexSearchLeft(Tree *tree, char* list[], int nlist, char lex[], char pret[]);
int lexSearchRight(Tree *tree, char* list[], int nlist, char lex[], char pret[]);
void  lexSearchLeftPar(Tree *tree, char* list[], int nlist, char lex[], char pret[], int *found);
void lexSearchRightPar(Tree *tree, char* list[], int nlist, char lex[], char pret[], int *found);
int isLabel(char* synT, char *label);

char* semanticTag(Tree *tree);
int baseVerb(char *label);
int findLeafVerb(Tree *tree);
void findVBinNPB(Tree *tree);
void findVBinNPBRec(Tree *tree);

Tree *createSingleNode(char *label, Tree *tree);
int coordinatedPhrase(Tree *tree, Tree **p, Tree **l, Tree **r);
int coordinatedLabel(Tree *tree, Tree *parent, Tree **l, Tree **r);
void coordSynT(Tree *tree, char synT[]);
int equvalentLabel(char *synT1, char *synT2);
int spuriousLabel(char *synT);
int pseudoCoordinationLabel(Tree *tree);
int headNextRight(Tree *tree, Tree *head);
int addNPB(char *label);
int preTNoun(char *label);
int getLeaves(Tree *tree, char *leaf[], int i);
int nodeNumber(Tree *tree);
int leafNumber(Tree *tree);
int height(Tree *tree);
int width(Tree *tree);
int countArity(Tree *tree, int c);
int countTreeLeaves(Tree *tree);
int hasPOSLeaf(Tree *tree, char *pos);

void writeTree(Tree *tree, int indent, int tab);
void writeTreeSingleLine(Tree *tree, int probFlag);
void writeLEXPOSLeaves(Tree *tree);
void writePOSLeaves(Tree *tree);
int writeLeaves(Tree *tree, int flag);
void writePOSClusters(Tree *tree);
void fwritePOSLeaves(FILE *f, Tree *tree);
int fwriteLeaves(FILE *f, Tree *tree, int flag);

void fwriteNodeNewline(FILE *f, Tree *tree);

Tree* deepCopy(Tree *tree, Tree *siblingL);
void freeTree(Tree *tree);

int sameString(char *ts, char *ds);
int sameCompactString(char *ts, char *ds);
int almostEqual(char *s1, char *s2);
void getLex(char *lex, char *fullex, int lowercaseFlag);
int getBLex(char *lex);
int getELex(char *lex);
int isPunctuation(char *s);
int dominatesSynTNP(Tree *tree);

int ruleHead(char *node);
int getHead(char parent[], char mod[][SLEN], int nmod);
int lexSearchLeftInList(char mod[][SLEN], int nmod, char* list[], int nlist);
int lexSearchRightInList(char mod[][SLEN], int nmod, char* list[], int nlist);
int lexSearchLeftParInList(char mod[][SLEN], int nmod, char* list[], int nlist);
int lexSearchRightParInList(char mod[][SLEN], int nmod, char* list[], int nlist);

#endif
