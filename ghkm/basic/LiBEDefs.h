// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
/** @file LiBEDefs.H
*   Variable or type definitions.
*/
#ifndef _LiBEDefs_H_
#define _LiBEDefs_H_

#define MAX_NGRAM_ORDER 100

/** The ID of events like terminal, nonterminal, etc. */
typedef unsigned int  EID;

/** The type for positions like word position in a sentence. */
typedef short POSITTYPE;


/** The language dimension type. */
typedef unsigned int DIMTYPE;


//! The type assigned to SCORE is used for all the numeric scores in ScoreSet and its descendants.
typedef float SCORET;

#define ZEROSCORET 0.000000000000000000

#ifndef  _BasicDefs_H_

//! Value of type SCORE with semantics "zero."
#define ZEROSCORE 0.000000000000000000

//! Value of type SCORE with semantics "one."
#define UNITYSCORE 1.00000000000000000

//! Value of type SCORE with semantics "one half."
#define HALFSCORE 0.500000000000000000

//! The base for the logs used in the logs and exps methods, and also the entropy and perplexity methods (in MultinomialDistribution).
#define LOGBASE exp(UNITYSCORE)

//! A tiny value used as the margin of error in comparisons of real numbers.
#define TINY 0.0000000001

#endif

#endif
