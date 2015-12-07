/* Copyright (c) 2002 by David Chiang. All rights reserved.*/

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_STRING_H
# include <string.h>
#endif

#include "pattern_op.h"

pattern_op new_pattern_op(int type, char *s) {
  pattern_op pop = malloc(sizeof *pop);

  pop->type = type;
  if (s != NULL)
    pop->s = strdup(s);
  else
    pop->s = NULL;

  pop->state = STATE_INITIAL;
  pop->simple = -1;
  pop->min_length = 0;

  return pop;
}

pattern_op pattern_op_copy(pattern_op pop) {
  pattern_op new = malloc(sizeof *new);

  new->type = pop->type;
  if (pop->s != NULL)
    new->s = strdup(pop->s);
  else
    new->s = NULL;

  new->state = pop->state;
  new->simple = pop->simple;
  new->min_length = pop->min_length;

  return new;
}

void pattern_op_delete(pattern_op pop) {
  free(pop->s);
  free(pop);
}

int pattern_op_to_string(pattern_op pop, char *s, int n) {
  char buf[100];
  switch (pop->type) {
  case OP_CONC_LEFT: strncpy(s, ">", n); break;
  case OP_CONC_RIGHT: strncpy(s, "<", n); break;
  case OP_ALT: strncpy(s, "|", n); break;
  case OP_STAR_LEFT_MAX: strncpy(s, ">*", n); break;
  case OP_STAR_RIGHT_MAX: strncpy(s, "<*", n); break;
  case OP_STAR_LEFT_MIN: strncpy(s, ">*?", n); break;
  case OP_STAR_RIGHT_MIN: strncpy(s, "<*?", n); break;
  case OP_SET: strncpy(s, "+", n); strncat(s, pop->s, n-strlen(s)); break;
  case OP_RESET: strncpy(s, "-", n); strncat(s, pop->s, n-strlen(s)); break;
  case OP_ATOM: strncpy(s, pop->s, n); break;
  case OP_EMPTY: strncpy(s, "<empty>", n); break;
  case OP_DOT: strncpy(s, ".", n); break;

  case OP_INTERSECTION: strncpy(s, "&", n); break;
  case OP_UNION: strncpy(s, ",", n); break;
  case OP_COMPLEMENT: strncpy(s, "!", n); break;
  }

  sprintf(buf, "[%d]", pop->min_length);
  strncat(s, buf, n-strlen(buf));
  
  return 0;
}

