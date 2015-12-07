/* Copyright (c) 2002 by David Chiang. All rights reserved.*/

#ifndef PATTERN_H
#define PATTERN_H

#include "node.h"
#include "pattern_op.h"

node new_pattern(pattern_op pop, node arg1, node arg2);
void pattern_delete(node pat);
int pattern_simple(node pat);
int pattern_label_match(node pat, node child);
int pattern_match(node pattern, node parent);
int pattern_expand_star(node pat);

#endif
