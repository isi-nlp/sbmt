#ifndef FILTERNGRAM_HPP
#define FILTERNGRAM_HPP

#include "Vocab.h"
#include "Ngram.h"
#include "NgramStats.h"

Ngram *make_filter(File &file, Vocab &vocab, int order);

class FilterNgram : public Ngram {
public:
  FilterNgram(Vocab &vocab, unsigned int order);
  virtual Boolean read(File &file, Ngram &filter);
};



#endif
