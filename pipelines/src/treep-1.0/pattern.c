/* Copyright (c) 2002 by David Chiang. All rights reserved.*/

#include "config.h"

#include <stdio.h>
#ifdef HAVE_STRING_H
# include <string.h>
#endif

#include "node.h"
#include "pattern.h"
#include "label.h"

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))

extern int debug;

void pattern_reset(node pat) {
  int i, n;
  pattern_op pop = pat->contents;

  if (pop->type == OP_ALT && pop->state != STATE_FINAL) {
    n = node_n_children(pat);
    for (i=0; i<=pop->state && i<n; i++)
      pattern_reset(node_child(pat, i));
  } else if (pop->type == OP_ALT || pop->type == OP_CONC_LEFT || pop->type == OP_CONC_RIGHT || pop->type == OP_SET || pop->type == OP_RESET) {
    n = node_n_children(pat);
    for (i=0; i<n; i++)
      pattern_reset(node_child(pat, i));
  }
  
  pop->state = STATE_INITIAL;
}

int pattern_simple(node pat) {
  pattern_op pop = pat->contents;
  int i, n;

  if (pop->simple < 0) {
    if (pop->type == OP_ALT ||
	pop->type == OP_STAR_LEFT_MAX ||
	pop->type == OP_STAR_RIGHT_MAX ||
	pop->type == OP_STAR_LEFT_MIN ||
	pop->type == OP_STAR_RIGHT_MIN) {
      pop->simple = 0;
    } else {
      pop->simple = 1;
      n = node_n_children(pat);
      for (i=0; i<n; i++)
	pop->simple = pop->simple && pattern_simple(node_child(pat, i));
    }
  }

  return pop->simple;
}

node new_pattern(pattern_op pop, node arg1, node arg2) {
  node new = new_node(pop);
  pattern_op pop1, pop2;

  if (arg1) {
    node_insert_child(new, 0, arg1);
    pop1 = arg1->contents;
  }
  if (arg2) {
    node_insert_child(new, 1, arg2);
    pop2 = arg2->contents;
  }

  /* Compute min_length */
  switch (pop->type) {
  case OP_CONC_LEFT:
  case OP_CONC_RIGHT:
    pop->min_length = pop1->min_length + pop2->min_length;
    break;
  case OP_ALT:
    pop->min_length = MIN(pop1->min_length, pop2->min_length);
    break;
  case OP_EMPTY:
  case OP_STAR_LEFT_MAX:
  case OP_STAR_RIGHT_MAX:
  case OP_STAR_LEFT_MIN:
  case OP_STAR_RIGHT_MIN:
    pop->min_length = 0;
    break;
  case OP_SET:
  case OP_RESET:
    pop->min_length = pop1->min_length;
    break;
  default: /* label pattern */
    pop->min_length = 1;
    break;
  }
  return new;
}

void pattern_delete(node pat) {
  tree_delete(pat, (DELETE)pattern_op_delete);
}

void pattern_dump(node pattern) {
  pattern_op pop = pattern->contents;
  switch(pop->type) {
  case OP_CONC_LEFT:
  case OP_CONC_RIGHT:
    pattern_dump(node_child(pattern, 0));
    printf(" ");
    pattern_dump(node_child(pattern, 1));
    break;
  case OP_ALT:
    if (pop->state >= 0)
      pattern_dump(node_child(pattern, pop->state));
    else
      printf("|");
    break;
  case OP_EMPTY:
    break;
  case OP_SET:
    pattern_dump(node_child(pattern, 0));
    printf("+%s", pop->s);
    break;
  case OP_RESET:
    pattern_dump(node_child(pattern, 0));
    printf("-%s", pop->s);
    break;
  default:
    printf("{");
    tree_print(stdout, pattern, (TO_STRING)pattern_op_to_string);
    printf("}");
  }
}

int pattern_label_match(node pat, node child) {
  pattern_op pop = pat->contents;

  switch (pop->type) {
  case OP_ATOM:
    return label_match(child->contents, pop->s);

  case OP_UNION:
    return pattern_label_match(node_child(pat, 0), child) || pattern_label_match(node_child(pat, 1), child);

  case OP_INTERSECTION:
    return pattern_label_match(node_child(pat, 0), child) && pattern_label_match(node_child(pat, 1), child);

  case OP_COMPLEMENT:
    return !pattern_label_match(node_child(pat, 0), child); 

  case OP_DOT:
    return 1;

  default:
    fprintf(stderr, "Unimplemented operator (%d)\n", pop->type);
    return 0;
  }
}

int pattern_expand_star(node pat) {
  int type;
  node conc, copy, arg, star;

  if (debug)
    fprintf(stderr, "pattern_expand_star()\n");

  arg = node_child(pat, 0);
  node_detach(arg);
  type = ((pattern_op)pat->contents)->type;

  ((pattern_op)pat->contents)->type = OP_ALT;

  if (type == OP_STAR_LEFT_MAX || type == OP_STAR_LEFT_MIN) {
    conc = new_node(new_pattern_op(OP_CONC_LEFT, NULL));
  } else {
    conc = new_node(new_pattern_op(OP_CONC_RIGHT, NULL));
  }
  node_insert_child(pat, 0, conc);

  if (type == OP_STAR_LEFT_MAX || type == OP_STAR_RIGHT_MAX)
    node_insert_child(pat, 1, new_node(new_pattern_op(OP_EMPTY, NULL)));
  else
    node_insert_child(pat, 0, new_node(new_pattern_op(OP_EMPTY, NULL)));

  copy = tree_copy(arg, (ENDO)pattern_op_copy);
  if (((pattern_op)copy->contents)->min_length == 0)
    ((pattern_op)copy->contents)->min_length = 1;
  node_insert_child(conc, 0, copy);
  ((pattern_op)conc->contents)->min_length = ((pattern_op)copy->contents)->min_length;

  star = new_node(new_pattern_op(type, NULL));

  if (type == OP_STAR_LEFT_MAX || type == OP_STAR_LEFT_MIN)
    node_insert_child(conc, 0, star);
  else
    node_insert_child(conc, 1, star);

  node_insert_child(star, 0, arg);

  return 0;
}

int pattern_match_empty(node pattern, node parent, int *starts, int *stops) {
  int flag, i, n;
  flag = 0;

  n = node_n_children(parent);
  for (i=0; i<=n; i++)
    if (starts[i] && stops[i])
      flag = 1;
  if (flag) {
    for (i=0; i<=n; i++)
      if (!(starts[i] && stops[i]))
	starts[i] = stops[i] = 0;
    return 0;
  } else
    return -1;
}

int pattern_match_symbol(node pattern, node parent, int *starts, int *stops) {
  int flag, i, n;
  static int *match = NULL, match_n;

  flag = 0;
  n = node_n_children(parent);

  if (match == NULL) {
    match = malloc(2*n * sizeof(int *));
    match_n = 2*n;
  } else if (n > match_n) {
    match = realloc(match, 2*n * sizeof(int *));
    match_n = 2*n;
  }

  for (i=0; i<n; i++)
    if (starts[i] && stops[i+1] && pattern_label_match(pattern, node_child(parent, i))) {
      flag = 1;
      match[i] = 1;
    } else
      match[i] = 0;

  if (flag) {
    starts[n] = stops[0] = 0;
    for (i=0; i<n; i++)
      starts[i] = stops[i+1] = match[i];
    return 1;
  } else {
    return -1;
  }
}

/* If no match, does not touch starts or stops. */
int pattern_match_helper(node pattern, node parent, int *starts, int *stops, int advance) {
  int i, n, sub, sub2;
  int imin, jmax;
  int *splits, *save;
  pattern_op pop = pattern->contents;

  if (pop->state == STATE_FINAL)
    return -1;

  n = node_n_children(parent);

  imin = n+1;
  for (i=0; i<=n; i++)
    if (starts[i]) {
      imin = i;
      break;
    }

  jmax = -1;
  for (i=n; i>=0; i--)
    if (stops[i]) {
      jmax = i;
      break;
    }

  if (imin > jmax)
    return -1;

  switch (pop->type) {

  case OP_CONC_LEFT:
  case OP_CONC_RIGHT:

    splits = calloc(n+1, sizeof(int));
    save = calloc(n+1, sizeof(int));

    imin += ((pattern_op)node_child(pattern, 0)->contents)->min_length;
    jmax -= ((pattern_op)node_child(pattern, 1)->contents)->min_length;

    for (i=imin; i<=jmax; i++)
      splits[i] = 1;

    if (pop->type == OP_CONC_LEFT) {

      for (i=0; i<=n; i++)
	save[i] = starts[i];
      
      if (pop->state == STATE_INITIAL && advance) {
	sub = pattern_match_helper(node_child(pattern, 0), parent, starts, splits, advance);
	pop->state = 0;
      } else
	sub = pattern_match_helper(node_child(pattern, 0), parent, starts, splits, 0);
      
      while (sub >= 0) {
	sub2 = pattern_match_helper(node_child(pattern, 1), parent, splits, stops, advance);
	if (sub2 >= 0) {
	  for (i=0; i<=n; i++)
	    if (starts[i] && !splits[i+sub])
	      starts[i] = 0;
	  free(splits);
	  free(save);
	  return sub+sub2;
	} else if (!advance)
	  break;
	
	/* No right-side match -- try the next left-side. */
	
	for (i=0; i<=n; i++)
	  starts[i] = save[i];
	for (i=imin; i<=jmax; i++)
	  splits[i] = 1;
	sub = pattern_match_helper(node_child(pattern, 0), parent, starts, splits, advance);
	pattern_reset(node_child(pattern, 1));
      }

      /* Ran out of left-side matches. */
      for (i=0; i<=n; i++)
	starts[i] = save[i];
      
    } else if (pop->type == OP_CONC_RIGHT) {

      for (i=0; i<=n; i++)
	save[i] = stops[i];

      if (pop->state == STATE_INITIAL && advance) {
	sub2 = pattern_match_helper(node_child(pattern, 1), parent, splits, stops, advance);
	pop->state = 0;
      } else
	sub2 = pattern_match_helper(node_child(pattern, 1), parent, splits, stops, 0);
      
      while (sub2 >= 0) {
	sub = pattern_match_helper(node_child(pattern, 0), parent, starts, splits, advance);
	if (sub >= 0) {
	  for (i=0; i<=n; i++)
	    if (stops[i] && !splits[i-sub2])
	      stops[i] = 0;
	  free(splits);
	  free(save);
	  return sub+sub2;
	} else if (!advance)
	  break;
	
	for (i=0; i<=n; i++)
	  stops[i] = save[i];
	for (i=imin; i<=jmax; i++)
	  splits[i] = 1;
	sub2 = pattern_match_helper(node_child(pattern, 1), parent, splits, stops, advance);
	pattern_reset(node_child(pattern, 0));
      }
      
      for (i=0; i<=n; i++)
	stops[i] = save[i];
    }
    
    if (advance)
      pop->state = STATE_FINAL;

    free(splits);
    free(save);
    return -1;

  case OP_ALT:
    if (pop->state == STATE_INITIAL && advance)
      pop->state = 0;

    while (pop->state < node_n_children(pattern)) {
      sub = pattern_match_helper(node_child(pattern, pop->state), parent, starts, stops, advance);
      
      if (sub >= 0)
	return sub;
      else if (!advance)
	break;

      pop->state++;
    }

    if (advance)
      pop->state = STATE_FINAL;
    return -1;

  case OP_STAR_LEFT_MAX:
  case OP_STAR_RIGHT_MAX:
  case OP_STAR_LEFT_MIN:
  case OP_STAR_RIGHT_MIN:
    pattern_expand_star(pattern);
    return pattern_match_helper(pattern, parent, starts, stops, advance);

  case OP_SET:
  case OP_RESET:
    return pattern_match_helper(node_child(pattern, 0), parent, starts, stops, advance);

  case OP_EMPTY:
    if (advance) {
      if (pop->state == STATE_INITIAL)
	pop->state = 0;
      else {
	pop->state = STATE_FINAL;
	return -1;
      }
    }
    return pattern_match_empty(pattern, parent, starts, stops);
  
  default: /* label pattern */
    if (advance) {
      if (pop->state == STATE_INITIAL)
	pop->state = 0;
      else {
	pop->state = STATE_FINAL;
	return -1;
      }
    }

    return pattern_match_symbol(pattern, parent, starts, stops);
  }
}


int pattern_mark_helper(node pattern, node parent, int i) {
  int j;
  pattern_op pop = pattern->contents;

  switch (pop->type) {
  case OP_CONC_LEFT:
  case OP_CONC_RIGHT:
    i = pattern_mark_helper(node_child(pattern, 0), parent, i);
    i = pattern_mark_helper(node_child(pattern, 1), parent, i);
    return i;
  case OP_ALT:
    return pattern_mark_helper(node_child(pattern, pop->state), parent, i);
  case OP_EMPTY:
    return i;
  case OP_SET:
    j = pattern_mark_helper(node_child(pattern, pop->state), parent, i);
    for (; i<j; i++)
      label_set(node_child(parent, i)->contents, pop->s);
    return j;
  case OP_RESET:
    j = pattern_mark_helper(node_child(pattern, pop->state), parent, i);
    for (; i<j; i++)
      label_reset(node_child(parent, i)->contents, pop->s);
    return j;
  default:
    return i+1;
  }
}

int pattern_mark(node pattern, node parent) {
  return pattern_mark_helper(pattern, parent, 0);
}

int pattern_match(node pattern, node parent) {
  int *starts = calloc(node_n_children(parent)+1, sizeof(int)),
    *stops = calloc(node_n_children(parent)+1, sizeof(int));
  int result;
  
  pattern_reset(pattern);
  starts[0] = 1;
  stops[node_n_children(parent)] = 1;
  result = pattern_match_helper(pattern, parent, starts, stops, 1);

  free(starts);
  free(stops);

  if (result < 0)
    return 0;

  if (debug) {
    pattern_dump(pattern);
    printf("\n");
  }

  pattern_mark(pattern, parent);

  return 1;
}

