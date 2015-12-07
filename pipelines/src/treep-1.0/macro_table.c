/* Copyright (c) 2002 by David Chiang. All rights reserved.*/

#include "config.h"

#include <stdio.h>
#ifdef HAVE_STRING_H
# include <string.h>
#endif

#include "marker.h"

#include "node.h"
#include "macro_table.h"
#include "pattern.h"

macro_table new_macro_table(void) {
  macro_table g = malloc(sizeof (*g));

  g->max_productions = 10;
  g->n_productions = 0;
  g->lhs = malloc(g->max_productions*sizeof(char *));
  g->rhs = malloc(g->max_productions*sizeof(node));

  return g;
}

void macro_table_delete(macro_table g) {
  int i;
  for (i=0; i<g->n_productions; i++) {
    free(g->lhs[i]);
    tree_delete(g->rhs[i], (DELETE)pattern_op_delete);
  }
  free(g->lhs);
  free(g->rhs);
  free(g);
}

int macro_table_add(macro_table g, char *lhs, node rhs) {
  if (g->n_productions == g->max_productions) {
    g->max_productions *= 2;
    g->lhs = realloc(g->lhs, g->max_productions*sizeof(char *));
    g->rhs = realloc(g->rhs, g->max_productions*sizeof(node));
  }
  g->lhs[g->n_productions] = strdup(lhs);
  g->rhs[g->n_productions] = rhs;
  g->n_productions++;

  if (debug) {
    fprintf(stderr, "Defined: %s :=", lhs);
    tree_print(stderr, rhs, (TO_STRING)pattern_op_to_string);
    fprintf(stderr, "\n");
  }

  return 0;
}

node macro_table_lookup(macro_table g, char *lhs) {
  int i;
  for (i=0; i<g->n_productions; i++)
    if (!strcmp(g->lhs[i], lhs))
      return g->rhs[i];
  return NULL;
}
