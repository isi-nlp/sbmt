/*
score.h
David Chiang <chiang@isi.edu>

Copyright (c) 2004-2006 University of Maryland. All rights
reserved. Do not redistribute without permission from the author. Not
for commercial use.
*/

#ifndef SCORE_H
#define SCORE_H

extern int comps_n;

void comps_addto(int *comps1, int *comps2);
float compute_score(int *comps);

#endif
