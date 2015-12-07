/* Copyright (c) 2002 by David Chiang. All rights reserved.*/

#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "node.h"

typedef struct {
  node *lhs, *rhs;
  int n_productions, max_productions;
} *grammar;

grammar new_grammar(void);
void grammar_delete(grammar g);

int grammar_add(grammar g, node lhs, node rhs);
node grammar_lookup(grammar g, node lhs);

#endif
