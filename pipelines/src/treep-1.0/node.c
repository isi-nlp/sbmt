/* Copyright (c) 2001 by David Chiang. All rights reserved.*/

#include "config.h"

#include <stdio.h>
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#include <stdlib.h>
#include <ctype.h>

#include "generic.h"
#include "node.h"

#define MAX_LABEL_LENGTH 255
#define LPAREN '('
#define RPAREN ')'

node new_node(void *contents) {
  node cur;

  cur = malloc(sizeof(*cur));

  cur->children = NULL;
  cur->max_children = cur->n_children = 0;
  cur->parent = NULL;
  cur->order = -1;
  cur->contents = contents;
  cur->shared = 0;

  return cur;
}

int node_make_shared(node cur) {
  cur->shared = 1;
  cur->parent = NULL;
  cur->order = -1;

  return 0;
}

int node_shared(node cur) {
  return cur->shared;
}

void node_delete(node cur) {
  int i;

  node_detach(cur);
  for (i=0; i<cur->n_children; i++)
    cur->children[i]->parent = NULL;
  free(cur->children);
  free(cur);
}

/* The two primitive tree manipulation functions. All others should
   use these only. */

int node_insert_child(node parent, int i, node child) {
  int j;

  if (parent == NULL) {
    node_detach(child);
    return 0;
  }

#ifdef CYCLE_CHECK
  if (node_dominates(child, parent)) {
    fprintf(stderr, "node_insert_child(): creating domination cycle\n");
  }
#endif

  if (i > parent->n_children)
    return -1;
  if (i < 0)
    i = parent->n_children + 1 + i;

  if (!parent->children) {
    parent->max_children = 2;
    parent->children = malloc(parent->max_children * sizeof(node));
  }

  if (parent->n_children == parent->max_children) {
    parent->max_children *= 2;
    parent->children = realloc(parent->children, parent->max_children * sizeof(node));
  }

  for (j=parent->n_children; j>i; j--) {
    parent->children[j] = parent->children[j-1];
    if (parent->children[j] && !parent->children[j]->shared)
      parent->children[j]->order = j;
  }
  parent->n_children++;
  parent->children[j] = child;

  node_detach(child);

  if (child && !child->shared) {
    child->parent = parent;
    child->order = j;
  }

  return 0;
}

int node_delete_child(node parent, int i) {
  int j;

  if (i<0 || i>=parent->n_children)
    return -1;

  parent->n_children--;

  if (parent->children[i]) {
    parent->children[i]->parent = NULL;
    parent->children[i]->order = -1;
  }

  for (j=i; j<parent->n_children; j++) {
    parent->children[j] = parent->children[j+1];
    if (parent->children[j] && !parent->children[j]->shared)
      parent->children[j]->order = j;
  }
  return 0;
}

int node_supplant(node old, node new) {
  int order;

  if (!old->shared && old != new) {
    order = node_order(old);
    node_insert_child(node_parent(old), order, new);
    node_detach(old);
  }

  return 0;
}

/* Bug: doesn't work with shared subtrees */

int node_dominates(node anc, node desc) {
  if (anc == desc)
    return 1;
  if (desc == NULL)
    return 0;

  return node_dominates(anc, node_parent(desc));
}

int node_detach(node child) {
  if (child && child->parent != NULL)
    node_delete_child(child->parent, child->order);

  return 0;
}

int node_n_children(node parent) {
  if (parent == NULL)
    return -1;
  else 
    return parent->n_children;
}

node node_child(node parent, int i) {
  if (parent == NULL)
    return NULL;
  if (i < 0 && parent->n_children+i >= 0)
    return parent->children[parent->n_children+i];
  else if (i >= 0 && i < parent->n_children)
    return parent->children[i];
  else
    return NULL;
}

node node_parent(node child) {
  if (child != NULL)
    return child->parent;
  else
    return NULL;
}

node node_sister(node child, int i) {
  if (node_parent(child) == NULL || node_order(child)+i < 0)
    return NULL;
  else
    return node_child(node_parent(child), node_order(child)+i);
}

int node_order(node child) {
  if (child == NULL)
    return -1;
  return child->order;
}

void * node_contents(node cur) {
  if (cur == NULL)
    return NULL;
  return cur->contents;
}

/* Bug: doesn't -- and shouldn't -- work for shared subtrees */

node node_root(node cur) {
  if (cur->parent == NULL)
    return cur;
  else
    return node_root(cur->parent);
}

/* Doesn't copy shared subtrees */

node tree_copy(node root, void * (*f)(void *)) {
  node new;
  int i;

  if (root == NULL)
    return NULL;

  if (root->shared)
    return root;

  new = new_node((*f)(root->contents));

  new->max_children = root->max_children;
  if (new->max_children > 0)
    new->children = malloc(sizeof(node) * new->max_children);
  else
    new->children = NULL;
  new->n_children = root->n_children;
  for (i=0; i<root->n_children; i++) {
    new->children[i] = tree_copy(root->children[i], f);
    if (new->children[i] && !new->children[i]->shared) {
      new->children[i]->parent = new;
      new->children[i]->order = i;
    }
  }

  new->shared = root->shared;

  new->parent = NULL;
  new->order = -1;

  return new;
}

void tree_map(node root, void * (*f)(void *)) {
  int i;
  root->contents = (*f)(root->contents);
  for (i=0; i<root->n_children; i++)
    tree_map(root->children[i], f);
}

void tree_delete(node root, _DELETE(f)) {
  int i, n;
  (*f)(root->contents);
  n = node_n_children(root);
  for (i=n-1; i>=0; i--)
    tree_delete(node_child(root, i), f);
  node_delete(root);
}

/* Tree I/O functions. */

int skip_whitespace(FILE *fp) {
  int c;
  c = getc(fp);
  while (c != EOF && isspace(c))
    c = getc(fp);
  if (c != EOF) {
    ungetc(c, fp);
    return 0;
  } else
    return -1;
}

int read_symbol(FILE *fp, char *buf, size_t n) {
  int c, i;

  if (skip_whitespace(fp) < 0)
    return -1;

  i = 0;
  c = getc(fp);
  while (c != EOF && i<n-1 && !isspace(c) && c != LPAREN && c != RPAREN) {
    buf[i] = c;
    c = getc(fp);
    i++;
  }

  if (c != EOF) {
    ungetc(c, fp);
  }
  
  buf[i] = '\0';

  return i;

}

node read_tree(FILE *fp, void * (*nonterm_from_string)(const char *), void * (*term_from_string)(const char *)) {
  int c, i;
  char buf[MAX_LABEL_LENGTH];
  node cur;
  
  if (skip_whitespace(fp) < 0)
    return NULL;
  
  c = getc(fp);

  if (c == EOF)
    return NULL;
  else if (c == LPAREN) {
    read_symbol(fp, buf, sizeof(buf));
    cur = new_node((*nonterm_from_string)(buf));

    i = 0;
    c = getc(fp);
    while (c != RPAREN && c != EOF) {
      ungetc(c, fp);
      node_insert_child(cur, i, read_tree(fp, nonterm_from_string, term_from_string));
      skip_whitespace(fp);
      i++;
      c = getc(fp);
    }
    if (c == EOF) {
      fprintf(stderr, "read_tree(): end of file reached in middle of tree\n");
      return NULL;
    }
    return cur;
  } else if (c == RPAREN) {
    fprintf(stderr, "read_tree(): mismatched right parenthesis\n");
    return NULL;
  } else {
    ungetc(c, fp);
    read_symbol(fp, buf, sizeof(buf));
    cur = new_node((*term_from_string)(buf));
    return cur;
  }

}

int tree_print(FILE *fp, node cur, int (*to_string)(const void *, char *, int)) {
  char buf[MAX_LABEL_LENGTH];
  int n, i, result, retval;

  if (cur == NULL) {
    fputs("NIL", fp);
    return 0;
  }

  n = node_n_children(cur);

  if (n == 0) {
    result = (*to_string)(cur->contents, buf, sizeof(buf));
    if (result >= 0)
      fputs(buf, fp);
    else
      return -1;
  } else if (n > 0) {
    result = (*to_string)(cur->contents, buf, sizeof(buf));

    if (result >= 0) {
      putc(LPAREN, fp);
      fputs(buf, fp);
    }

    retval = -1;
    for (i=0; i<n; i++) {
      putc(' ', fp);
      if (tree_print(fp, node_child(cur, i), to_string) >= 0)
	retval = 0;
    }

    if (result >= 0)
      putc(RPAREN, fp);
  }

  return retval;
}

int pretty_print_helper(FILE *fp, node cur, int indent, int cursor, int (*to_string)(const void *, char *, int)) {
  char buf[MAX_LABEL_LENGTH];
  int n, i;

  for (i=0; i<indent-cursor; i++)
    putc(' ', fp);

  n = node_n_children(cur);

  if (n == 0) {
    (*to_string)(cur->contents, buf, sizeof(buf));
    fputs(buf, fp);
  } else if (n > 0) {
    putc(LPAREN, fp);
    (*to_string)(cur->contents, buf, sizeof(buf));
    fputs(buf, fp);
    putc(' ', fp);
    cursor = indent+2+strlen(buf);
    pretty_print_helper(fp, node_child(cur, 0), cursor, cursor, to_string);
    for (i=1; i<n; i++) {
      putc('\n', fp);
      pretty_print_helper(fp, node_child(cur, i), cursor, 0, to_string);
    }
    putc(RPAREN, fp);
  }

  return 0;
}

int tree_pretty_print(FILE *fp, node cur, int (*to_string)(const void *, char *, int)) {
  pretty_print_helper(fp, cur, 0, 0, to_string);

  return 0;
}

/* Functions for string-labeled nodes */

int string_to_string(const char *s1, char *s2, int n) {
  strncpy(s2, s1, n);
  return 0;
}

void * string_from_string(const char *s) {
  return strdup(s);
}
