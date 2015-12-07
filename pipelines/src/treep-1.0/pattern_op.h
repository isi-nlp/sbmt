/* Copyright (c) 2002 by David Chiang. All rights reserved.*/

#ifndef PATTERN_OP_H
#define PATTERN_OP_H

typedef struct {

#define OP_CONC_LEFT 0
#define OP_CONC_RIGHT 1
#define OP_ALT 2
#define OP_STAR_LEFT_MAX 3
#define OP_STAR_RIGHT_MAX 4
#define OP_STAR_LEFT_MIN 5
#define OP_STAR_RIGHT_MIN 6

#define OP_UNION 7
#define OP_INTERSECTION 8
#define OP_COMPLEMENT 9

#define OP_SET 10
#define OP_RESET 11
#define OP_ATOM 12
#define OP_DOT 13
#define OP_EMPTY 14

  int type;
  char *s;

  int simple, state;
  int min_length, max_length;

#define STATE_INITIAL -1
#define STATE_FINAL -2

} *pattern_op;

pattern_op new_pattern_op(int type, char *s);
void pattern_op_delete(pattern_op pop);
pattern_op pattern_op_copy(pattern_op pop);
int pattern_op_to_string(pattern_op pop, char *s, int n);

#endif
