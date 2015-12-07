#ifndef BUILTIN_INFOS__SBLM_NGRAM_HPP
#define BUILTIN_INFOS__SBLM_NGRAM_HPP

# include <sbmt/edge/any_info.hpp>
# include <boost/program_options.hpp>
# include <boost/shared_ptr.hpp>
# include <string>
# include <sbmt/search/block_lattice_tree.hpp>
# include <sbmt/token.hpp>
# include <sbmt/token/indexed_token.hpp>
# include <sbmt/edge/component_scores.hpp>
# include <gusc/generator/single_value_generator.hpp>
# include <sbmt/hash/oa_hashtable.hpp>
# include <graehl/shared/assoc_container.hpp>

namespace sbmt {

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(sblm_bigram_domain,"sblm_bigram",root_domain);

class sblm_ngram_info : public info_base<sblm_ngram_info>
{
private:
  indexed_token lr[2];
public:
  sblm_ngram_info() {
    lr[0]=lr[1]=indexed_token();
  }
  template <class C, class T, class TF>
  std::basic_ostream<C,T>& print_self(std::basic_ostream<C,T>& o, TF& tf) const {
    print(o,lr[0],tf);
    o<<'..';
    print(o,lr[1],tf);
    return o;
  }
  //! compare the context words
  bool equal_to(info_type const& other) const
  {
    return lr[0]==other.lr[0] && lr[1]==other.lr[1];
  }
  std::size_t hash_value() const {
    return boost::hash_combine(lr[0].hash_value(),lr[1].hash_value());
  }
};


typedef unsigned tag_id;
const score_t zero_prob=1e-10;
class tag_unigram {
  typedef tag_id K;
  typedef std::pair<score_t,score_t> V; //prob,bow
#  typedef oa_hash_map<K,V> M;
  typedef std::vector<V> M;
  M m;
  score_t prob(K w) const {
    if (w<m.size())
      return m[w].first;
    return zero_prob;
  }
  score_t bow(K w) const {
    if (w<m.size()) {
      score_t r=return m[w].second;
      return r.is_zero() ? score_t(as_one()) : r;
    }
    return score_t(as_one());
  }

};

class tag_ngram {
  typedef std::pair<tag_id,tag_id> K;
  typedef score_t V;
  typedef oa_hash_map<K,V> M;
  M probs;
  token_unigram uni; // ngrams include interpolated prob already.
  score_t prob(tag_id ctx,tag_id w) const {
    M::iterator i=probs.find(K(ctx,w));
    return i==probs.end() ? uni.bow(ctx)*uni.prob(w) : *i;
  }
};



class sblm_ngram_info_factory
{
private:
public:
  typedef sblm_ngram_info info_type;
  typedef token_ngram lm_type;

  score_t heuristic_score(info_type const& it) const  {
    return 1;
  }
  clm_info_factory( indexed_token_factory& dict
                    , boost::shared_ptr<lm_type> lm)
  {
  }
};


}

#endif
