#ifndef BIGLMWRAP_H
#define BIGLMWRAP_H

#ifdef __cplusplus
  extern "C" {
    using namespace biglm;
#else
    typedef struct lm lm; /* dummy type to stand in for class */
#endif

lm * new_biglm(const char *filename, int override_unk, float p_unk);
void biglm_delete(lm * biglm);
float biglm_wordProb(lm * biglm, unsigned *words, int n);
float biglm_wordProbs(lm * biglm, unsigned *words, int n, int start, int stop);
unsigned biglm_lookup_word(lm * biglm, const char *s);
int biglm_get_order(lm * biglm);

#ifdef __cplusplus
  }
#endif

#endif

