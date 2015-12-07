/* Copyright (c) 2002 by David Chiang. All rights reserved.*/

#include "config.h"

#include <stdio.h>

#include "marker.h"

#include "node.h"
#include "grammar.h"
#include "pattern.h"

grammar new_grammar(void) {
  grammar g = malloc(sizeof (*g));

  g->max_productions = 10;
  g->n_productions = 0;
  g->lhs = malloc(g->max_productions*sizeof(node));
  g->rhs = malloc(g->max_productions*sizeof(node));

  return g;
}

void grammar_delete(grammar g) {
  int i;
  for (i=0; i<g->n_productions; i++) {
    tree_delete(g->lhs[i], (DELETE)pattern_op_delete);
    tree_delete(g->rhs[i], (DELETE)pattern_op_delete);
  }
  free(g->lhs);
  free(g->rhs);
  free(g);
}

int grammar_add(grammar g, node lhs, node rhs) {
  if (g->n_productions == g->max_productions) {
    g->max_productions *= 2;
    g->lhs = realloc(g->lhs, g->max_productions*sizeof(node));
    g->rhs = realloc(g->rhs, g->max_productions*sizeof(node));
  }
  g->lhs[g->n_productions] = lhs;
  g->rhs[g->n_productions] = rhs;
  g->n_productions++;

  if (debug) {
    fprintf(stderr, "Added: ");
    tree_print(stderr, lhs, (TO_STRING)pattern_op_to_string);
    fprintf(stderr, " -> ");
    tree_print(stderr, rhs, (TO_STRING)pattern_op_to_string);
    fprintf(stderr, "\n");
  }

  return 0;
}

node grammar_lookup(grammar g, node lhs) {
  int i;
  for (i=0; i<g->n_productions; i++)
    if (pattern_label_match(g->lhs[i], lhs))
      return g->rhs[i];
  return NULL;
}
