/* Copyright (c) 2002 by David Chiang. All rights reserved.*/

#include "config.h"

#include <stdio.h>

#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
# endif
char *strchr();
#endif

#include "label.h"

char *delimiters = "-=|";

label new_label(void) {
  label l = malloc(sizeof(*l));

  l->base = NULL;
  l->n_tags = 0;
  l->max_tags = 2;
  l->tags = malloc(l->max_tags * sizeof(char *));

  return l;
}

void label_delete(label l) {
  int i;
  for (i=0; i<l->n_tags; i++)
    free(l->tags[i]);
  free(l->base);
  free(l->tags);
  free(l);
}

/* Allows duplicates */
/* l owns tag */
int label_add_tag(label l, char *tag) {
  l->n_tags++;
  while (l->n_tags > l->max_tags) {
    l->max_tags *= 2;
    l->tags = realloc(l->tags, (l->max_tags)*sizeof(*l->tags));
  }
  l->tags[l->n_tags-1] = tag;

  return 0;
}

int label_to_string(label l, char *s, size_t n) {
  int i, len;

  strncpy(s, l->base, n);
  len = strlen(l->base);
  s += len;
  n -= len;

  for (i=0; i<l->n_tags; i++) {
    if (!strchr(delimiters, l->tags[i][0]))
      fprintf(stderr, "warning: tag %s doesn't start with delimiter\n", l->tags[i]);

    len = strlen(l->tags[i]);
    if (len+1 > n) /* +1 for terminating null */
      return -1;

    strncpy(s, l->tags[i], n);
    s += len;
    n -= len;
  }
  
  return 0;
}

label label_from_string(const char *s) {
  label l = new_label();
  int start, end, len;
  char *tag;

  len = strlen(s);
  start = 0;

  while (start<len) {
    /* No empty tags. */
    end = start+1;
    while (!(s[end] == '\0' || strchr(delimiters, s[end])))
      end++;
    
    /* Allow trailing delimiter characters. */
    while (s[end] != '\0' && 
	   (s[end+1] == '\0' || strchr(delimiters, s[end+1])))
      end++;
    
    tag = malloc((end-start+1)*sizeof(char));
    strncpy(tag, s+start, end-start);
    tag[end-start] = '\0';
    if (start == 0) {
      l->base = tag;
    } else {
      label_add_tag(l, tag);
    }

    start = end;
  }

  if (l->base == NULL)
    l->base = strdup("");

  return l;
}

int label_match(label l, const char *s) {
  int i;
  if (l->base && !strcmp(l->base, s))
    return 1;
  for (i=0; i<l->n_tags; i++)
    if (!strcmp(l->tags[i], s))
      return 1;
  return 0;
}

int label_set(label l, const char *s) {
  if (label_match(l, s))
    return 0;
  else
    return label_add_tag(l, strdup(s));
}

int label_reset(label l, const char *s) {
  int i, k;

  if (!strcmp(l->base, s)) {
    fprintf(stderr, "label_reset(): cannot reset base category %s\n", s);
    return -1;
  }

  k = 0;
  for (i=0; i+k<l->n_tags; i++) {
    if (!strcmp(l->tags[i], s))
      k = 1;
    if (k > 0 && i+k<l->max_tags) {
      l->tags[i] = l->tags[i+k];
    }
  }
  l->n_tags -= k;
  return 0;
}



