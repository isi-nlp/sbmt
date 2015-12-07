#include "config.h"

#include <stdlib.h>
#ifdef HAVE_STRING_H
# include <string.h>
#endif

#ifndef HAVE_STRDUP
char * strdup(char *s) {
  char *dup = malloc(strlen(s)+1);
  if (dup) 
    strcpy(dup, s);
  return dup;
}
#endif
