#ifndef SBLM_LIMITS_H
#define SBLM_LIMITS_H

#define ThCnt 60
/* used to set the 0|1 flag in grammar.lex; currently ignored by model2.c when reading in */

#define WORD_ON_2_BYTES 0

#define SLEN       80
#define LLEN      300
#define NKEY      100
#define MAXVOC 300000
#define MAXTAGS    50
#define MAXSUBCATS 50
#define MAXC     5000
#define MAXNTS    129
/* MAXNTS not over 240, as starting with 241 we have special symbols (CC, PUNC, HEAD, etc.) */

#endif
