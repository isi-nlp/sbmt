#ifndef SBMT__NGRAM__null_ngram_lm_hpp
#define SBMT__NGRAM__null_ngram_lm_hpp
// if you want to use CLM w/o a normal LM, this will do (just holds a vocabulary)

#include <sbmt/token/in_memory_token_storage.hpp>
#include <sbmt/ngram/dynamic_ngram_lm.hpp>

namespace sbmt {

struct null_ngram_lm : public dynamic_ngram_lm
{
    typedef in_memory_token_storage tf_type;
    typedef dynamic_ngram_lm Base;
    tf_type tf;

    virtual void set_weights_raw(weight_vector const& weights,feature_dictionary& dict) {}

    typedef unsigned lm_id_type;
    typedef lm_id_type * iterator;
    typedef lm_id_type const* const_iterator;

    static inline score_t one()
    {
        return score_t(as_one());
    }


    null_ngram_lm(ngram_options const& opt_,std::istream &in,boost::filesystem::path const& relative_base=boost::filesystem::initial_path()) : Base("null",opt_)
    {
        Base::finish();
        set_ids();
    }

    virtual lm_id_type id_raw(const std::string &word,bool add_unk)
    {
        if (add_unk)
            return tf.get_index(word);
        else return tf.get_index_unk(word,unk_id);
/*        if (tf.has_token(word))
            return const_cast<tf_type const&>(tf).get_index(word);
        else
        return unk_id;*/
    }
    virtual std::string const& word_raw(lm_id_type id) const
    {
        return tf.get_token(id);
    }
    virtual score_t open_prob_len_raw(const_iterator ctx_and_w,unsigned len) const
    {
        return one();
    }
    virtual score_t find_bow_raw(const_iterator ctx,const_iterator end) const
    {
        return one();
    }
    virtual score_t find_prob_raw(const_iterator ctx,const_iterator end) const
    {
        return one();
    }
    virtual unsigned max_order_raw() const
    {
        return 1;
    }
    virtual bool set_option(std::string,std::string) {return false; }
  virtual vocab_generator vocab() const {
    return vocab_generator(vocab_range_generator(tf.ibegin(),tf.iend()));
  }
  virtual lm_id_type first_unused_id(unsigned) const {
    return tf.iend();
  }

};



}


#endif
