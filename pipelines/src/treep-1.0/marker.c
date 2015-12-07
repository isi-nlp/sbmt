/* Copyright (c) 2002 by David Chiang. All rights reserved.*/

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

#include "node.h"
#include "pattern.h"
#include "macro_table.h"
#include "grammar.h"
#include "label.h"

int debug = 0, pretty = 0, pedantic = 0;
extern char *delimiters;

extern int yydebug;
extern FILE *yyin;

macro_table macros;
grammar productions, *cascade;
char *filename;
int tree_number;


int process(node cur, grammar productions) {
  int i, n, j, flag;
  char buf[100];
  
  n = node_n_children(cur);

  /* terminal always OK */
  if (n == 0)
    return 0;

  flag = 0;
  for (j=0; j<productions->n_productions; j++) {
    if (pattern_label_match(productions->lhs[j], cur)) {
      if (pattern_match(productions->rhs[j], cur)) {
	flag = 1;
	break;
      }
    }
  }

  if (!flag) {
    fprintf(stderr, "warning: %s failed on sentence %d: ", filename, tree_number);
    label_to_string(cur->contents, buf, sizeof(buf));
    fprintf(stderr, "(%s", buf);
    for (i=0; i<n; i++) {
      label_to_string(node_child(cur, i)->contents, buf, sizeof(buf));
      fprintf(stderr, " %s", buf);
    }
    fprintf(stderr, ")\n");
    return -1;
  }

  for (i=0; i<n; i++)
    if (process(node_child(cur, i), productions) < 0)
      return -1;
  return 0;
}

void usage_error() {
  fprintf (stderr, "%s %s\nUsage: marker <options> <rule set> ...\n", PACKAGE, VERSION);
}

int main (int argc, char *argv[]) {
  node root;
  int i, n;
  int c;
  char buf[256];

  while ((c = getopt(argc, argv, "d:vps")) != EOF)
    switch (c) {
    case 'd': delimiters = strdup(optarg); break;
    case 'v': debug++; break;
    case 'p': pretty = 1;
    case 's': pedantic = 1;
    }

  n = argc-optind;

  if (n <= 0) {
    usage_error();
    fprintf(stderr, "You must specify at least one rule set.\n");
    exit(1);
  }

  if (debug >= 2)
    yydebug = 1;

  cascade = malloc(n*sizeof(grammar));
  for (i=0; i<n; i++) {
    filename = argv[i+optind];

    macros = new_macro_table();
    productions = cascade[i] = new_grammar();

    yyin = fopen(filename, "r");
    if (yyin == NULL) {
      sprintf(buf, "%s: couldn't open rule set %s", argv[0], filename);
      perror(buf);
      exit(1);
    }
    yyparse();

    macro_table_delete(macros);
  }

  tree_number=0;
  while ((root = read_tree(stdin, (FROM_STRING)label_from_string, (FROM_STRING)label_from_string)) != NULL) {
    tree_number++;

    for (i=0; i<n; i++) {
      filename = argv[i+optind]; /* for error reporting */
      process(root, cascade[i]);
    }

    if (pretty)
      tree_pretty_print(stdout, root, (TO_STRING)label_to_string);
    else
      tree_print(stdout, root, (TO_STRING)label_to_string);
    printf("\n");

    tree_delete(root, (DELETE)label_delete);
  }

  return 0;
}
