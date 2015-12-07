#ifndef SBMT__NGRAM__BIG_NGRAM_LM_HPP
#define SBMT__NGRAM__BIG_NGRAM_LM_HPP

#include <sbmt/ngram/dynamic_ngram_lm.hpp>
#include <sbmt/token/indexed_token.hpp>
#include <biglm/pagh/biglm.hpp>

namespace sbmt {

struct big_ngram_lm : public dynamic_ngram_lm, public biglm::lm
{
    typedef dynamic_ngram_lm Base;
    typedef biglm::lm C;
    typedef biglm::word_type word_type;
    typedef biglm::weight_type weight_type;

    C & concrete()
    {
        return *static_cast<C*>(this);
    }
    C const& concrete() const
    {
        return *static_cast<C const*>(this);
    }
    virtual bool set_option(std::string,std::string) {return false; }
    virtual void set_weights_raw(weight_vector const& weights,feature_dictionary& dict) {}

    typedef unsigned lm_id_type;
    typedef lm_id_type * iterator;
    typedef lm_id_type const* const_iterator;

    inline static score_t score(weight_type w)
    {
        //FIXME: need to convert IMPOSSIBLE to 0?  I think -HUGE_VAL should do it.
        return score_t(w,as_log10());
    }

    big_ngram_lm(ngram_options const& opt,std::istream &i
                 ,boost::filesystem::path const& relative_base=boost::filesystem::initial_path()
                 ,bool override_unk=0, weight_type p_unk=0.0)
        : Base("big",opt), C(parse_filename(i,relative_base),override_unk,p_unk)
    {
        Base::finish(); // FIXME: this now does sync_vocab_and_grammar, but before, big_ngram_lm never did.  should be ok.
    }

    virtual lm_id_type id_raw(const std::string &word,bool add_unk)
    {
        //TODO: handle add_unk (wrap ids > max)?
        return (lm_id_type)concrete().lookup_word(word);
    }
    virtual std::string const& word_raw(lm_id_type id) const
    {
        return concrete().lookup_id((word_type)id);
    }
    virtual score_t open_prob_len_raw(const_iterator ctx_and_w,unsigned len) const
    {
        shorten_ngram_len(ctx_and_w,len);
        return score(concrete().lookup_ngram((word_type *)ctx_and_w,(int)len));
    }
    virtual score_t find_bow_raw(const_iterator ctx,const_iterator end) const
    {
        return score(concrete().lookup_bow(ctx,(end-ctx)));
    }
    virtual score_t find_prob_raw(const_iterator ctx,const_iterator end) const
    {
//        shorten_ngram(ctx,end);
        return score(concrete().lookup_prob(ctx,(end-ctx)));
    }
    virtual unsigned max_order_raw() const
    {
        return concrete().get_order();
    }
  virtual vocab_generator vocab() const
  {
    return vocab_generator(vocab_range_generator(concrete().vocab_ibegin(),concrete().vocab_iend()));
  }
  virtual lm_id_type first_unused_id(unsigned) const {
    return concrete().vocab_iend();
  }


};

}


#endif
