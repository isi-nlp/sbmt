#ifndef BIGLM_HPP
#define BIGLM_HPP

#include <vector>
#include <string>
#include <cstdio>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/noncopyable.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include "cmph.hpp"

#include "quantizer.hpp"
#include "sliceable_bitset.hpp"
#include "types.hpp"

// forward declarations necessary for friend declaration in biglm::lm
void dump_mph_from_arpa(std::istream &file, const char *filename);
void dump_values_from_arpa(std::istream &file, const char *filename, int checksum_bits, const std::vector<quantizer> &pqs, const std::vector<quantizer> &bqs);

namespace biglm {

typedef unsigned int word_type;
typedef std::vector<word_type> ngram_type;

class lm : boost::noncopyable {
  int order;

  // vocab
  std::vector<std::string> vocab;
  cmph::mph<std::string> *vocab_mph;
  std::vector<cmph::mph<ngram_type> *> ngram_mph;
  word_type unk;


  // checksums for n-grams
  cmph::hash_state<ngram_type> *ngram_hash;

  // quantizers for probs and bows
  std::vector<quantizer> prob_quantizer, bow_quantizer;

  // values
  boost::uint32_t checksum_bits, prob_bits, bow_bits;
  std::vector<sliceable_bitset *> values;

  bool override_unk;
  weight_type p_unk;

  std::vector<boost::iostreams::mapped_file_source>
	save_maps;
  std::vector<char*> save_start;
  std::vector<size_t> save_length;
  size_type n_keys(int o) const {
    if (o == 1)
      return vocab_mph->size();
    else
      return ngram_mph[o-2]->size();
  }

  int value_bits(int o) const {
    int b = prob_bits;
    if (o > 1)
      b += checksum_bits;
    if (o < order)
      b += bow_bits;
    return b;
  }

  int lookup_qprob(const word_type *ngram, int o) const;
  int lookup_qbow(const word_type *ngram, int o) const;

  void read_mph(FILE *fp);
  void read_values(FILE *fp,std::string const&);

  lm() : vocab_mph(NULL), ngram_hash(NULL) {}

public:
  typedef std::vector<std::string>::const_iterator vocab_string_iterator;
  vocab_string_iterator vocab_string_begin() const {
      return vocab.begin();
  }

  vocab_string_iterator vocab_string_end() const {
        return vocab.end();
  }

  word_type vocab_ibegin() const {
    return 0;
  }
  word_type vocab_iend() const {
    return vocab.size();
  }

  static int debug;
  friend void ::dump_mph_from_arpa(std::istream &file, const char *filename);
  friend void ::dump_values_from_arpa(std::istream &file, const char *filename, int checksum_bits, const std::vector<quantizer> &pqs, const std::vector<quantizer> &bqs);

  lm(const std::string &filename, bool a_override_unk=0, weight_type a_p_unk=0.0);
  ~lm();

  word_type lookup_word(const std::string &s) const;
  const std::string & lookup_id(const word_type w) const {
    return vocab[w];
  }

  weight_type lookup_ngram(const word_type *ngram, int n) const;
  weight_type lookup_ngrams(const word_type *ngram, int n, int start, int stop) const;
  weight_type lookup_ngram_checked(const word_type *ngram, int n) const;
  weight_type lookup_ngram(const ngram_type &ngram) const { return lookup_ngram(&ngram[0], ngram.size()); }

  weight_type lookup_prob(const word_type *ngram, int n) const {
    int qp = lookup_qprob(ngram, n);
    if (qp >= 0)
      return prob_quantizer[n-1].decode(qp);
    else
      return IMPOSSIBLE;
  }
  weight_type lookup_prob(const ngram_type &ngram) const { return lookup_prob(&ngram[0], ngram.size()); }

  weight_type lookup_bow(const word_type *ngram, int n) const {
    int qb = lookup_qbow(ngram, n);
    if (qb >= 0)
      return bow_quantizer[n-1].decode(qb);
    else
      return IMPOSSIBLE;
  }
  weight_type lookup_bow(const ngram_type &ngram) const { return lookup_bow(&ngram[0], ngram.size()); }

  int get_order() const { return order; }

  bool check_ngram(const word_type *ngram, int start, int length) const;
};

}

#endif
