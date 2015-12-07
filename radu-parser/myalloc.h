#ifndef _MYALLOC_
#define _MYALLOC_ 

int**** myIntPPPPalloc(int n, int m, int p, int r);
int*** myIntPPPalloc(int n, int m, int p);
int** myIntPPalloc(int n, int m);
int** myIntPPpartalloc(int n);
int* myIntPalloc(int n);
void myIntPPfree(int **l, int n, int m);
void myIntPfree(int *l, int n);

double*** myDoublePPPalloc(int n, int m, int p);
double** myDoublePPalloc(int n, int m);
double* myDoublePalloc(int n);

#endif
