#include "biglm.hpp"

#include "biglmwrap.h"

lm * new_biglm(const char *filename, int override_unk, float p_unk) {
  std::string s(filename);
  return new lm(s, override_unk, p_unk);
}

void biglm_delete(lm * biglm) {
  delete biglm;
}

float biglm_wordProb(lm * biglm, unsigned *words, int n) {
  return (float)biglm->lookup_ngram(words, n);
}

float biglm_wordProbs(lm * biglm, unsigned *words, int n, int start, int stop) {
  return (float)biglm->lookup_ngrams(words, n, start, stop);
}

unsigned biglm_lookup_word(lm * biglm, const char *s) {
  std::string s1(s);
  return biglm->lookup_word(s1);
}

int biglm_get_order(lm * biglm) {
  return biglm->get_order();
}
