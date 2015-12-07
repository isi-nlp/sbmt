/* Copyright (c) 2002 by David Chiang. All rights reserved.*/

#ifndef MACRO_TABLE_H
#define MACRO_TABLE_H

#include "node.h"

typedef struct {
  char **lhs;
  node *rhs;
  int n_productions, max_productions;
} *macro_table;

macro_table new_macro_table(void);
void macro_table_delete(macro_table g);

int macro_table_add(macro_table g, char *lhs, node rhs);
node macro_table_lookup(macro_table g, char *lhs);

#endif
