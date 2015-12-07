#include "myalloc.h"

int**** myIntPPPPalloc(int n, int m, int p, int r)
{
  int i, ****l;

  if( (l = (int ****) calloc(n, sizeof(int ***)))== NULL )
    { fprintf(stderr, "Out of memory\n"); exit(12); }

  for(i=0; i<n; i++)
    l[i] = myIntPPPalloc(m, p, r);

  return l;
}

int*** myIntPPPalloc(int n, int m, int p)
{
  int i, ***l;

  if( (l = (int ***) calloc(n, sizeof(int **)))== NULL )
    { fprintf(stderr, "Out of memory\n"); exit(12); }

  for(i=0; i<n; i++)
    l[i] = myIntPPalloc(m, p);

  return l;
}

int** myIntPPalloc(int n, int m)
{
  int i, **l;

  if( (l = (int **) calloc(n, sizeof(int *)))== NULL )
    { fprintf(stderr, "Out of memory\n"); exit(12); }

  for(i=0; i<n; i++)
    l[i] = myIntPalloc(m);

  return l;
}

int** myIntPPpartlloc(int n)
{
  int i, **l;

  if( n==0 )
    l = NULL;
  else
    if( (l = (int **) calloc(n, sizeof(int *)))== NULL )
      { fprintf(stderr, "Out of memory\n"); exit(12); }

  return l;
}

int* myIntPalloc(int n)
{
  int *l;

  if(n==0)
    l = NULL;
  else
    if( (l = (int *) calloc(n, sizeof(int)))== NULL )
      { fprintf(stderr, "Out of memory\n"); exit(16); }

  return l;
}

void myIntPPfree(int **l, int n, int m)
{
  int i;

  for(i=0; i<n; i++)
    myIntPfree(l[i], m);
  
  free(l); 
}


void myIntPfree(int *l, int n)
{
  free(l); 
}

double*** myDoublePPPalloc(int n, int m, int p)
{
  int i;
  double ***l;

  if( (l = (double ***) calloc(n, sizeof(double **)))== NULL )
    { fprintf(stderr, "Out of memory\n"); exit(12); }

  for(i=0; i<n; i++)
    l[i] = myDoublePPalloc(m, p);

  return l;
}

double** myDoublePPalloc(int n, int m)
{
  int i;
  double **l;

  if( (l = (double **) calloc(n, sizeof(double *)))== NULL )
    { fprintf(stderr, "Out of memory\n"); exit(12); }

  for(i=0; i<n; i++)
    l[i] = myDoublePalloc(m);

  return l;
}

double* myDoublePalloc(int n)
{
  double *l;

  if(n==0)
    l = NULL;
  else
    if( (l = (double *) calloc(n, sizeof(double)))== NULL )
      { fprintf(stderr, "Out of memory\n"); exit(16); }

  return l;
}

