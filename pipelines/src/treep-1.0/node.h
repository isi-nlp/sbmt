/* Copyright (c) 2001 by David Chiang. All rights reserved.*/

#ifndef NODE_H
#define NODE_H

#include <stdio.h>
#include <stdlib.h>

#include "generic.h"

typedef struct _node {
  struct _node **children;
  int n_children, max_children;
  struct _node *parent;
  int order;
  int shared;
  void * contents;
} *node;

node new_node(void *contents);
void node_delete(node cur);
int node_insert_child(node parent, int i, node child);
int node_delete_child(node parent, int i);
int node_detach(node child);
int node_n_children(node parent);
int node_supplant(node old, node new);
int node_dominates(node anc, node desc);
node node_child(node parent, int i);
node node_sister(node child, int i);
node node_parent(node child);
int node_order(node child);
void * node_contents(node cur);
node node_root(node cur);

node tree_copy(node root, _ENDO(f));
void tree_map(node root, _ENDO(f));
void tree_delete(node root, _DELETE(f));

node read_tree(FILE *fp, _FROM_STRING(nonterm_from_string), _FROM_STRING(term_from_string));
int tree_print(FILE *fp, node root, _TO_STRING(to_string));
int tree_pretty_print(FILE *fp, node root, _TO_STRING(to_string));

int string_to_string(const char *s1, char *s2, int n);
void * string_from_string(const char *s);

#endif
