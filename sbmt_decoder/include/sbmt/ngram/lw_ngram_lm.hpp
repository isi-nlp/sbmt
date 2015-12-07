#ifndef SBMT_NGRAM__LW_NGRAM_LM_HPP
#define SBMT_NGRAM__LW_NGRAM_LM_HPP

#include <sbmt/ngram/dynamic_ngram_lm.hpp>
#include <sbmt/ngram/LWNgramLM.hpp>

namespace sbmt {

//FIXME: inheriting: public dynamify_ngram_lm<LWNgramLM_impl> didn't work.  why?
struct lw_ngram_lm : public dynamic_ngram_lm, public LWNgramLM_impl
{
//    typedef dynamify_ngram_lm<LWNgramLM_impl> Base;
    typedef dynamic_ngram_lm Base;
    typedef LWNgramLM_impl C;
    C & concrete()
    {
        return *static_cast<C*>(this);
    }
    C const& concrete() const
    {
        return *static_cast<C const*>(this);
    }

    typedef unsigned lm_id_type;
    typedef lm_id_type * iterator;
    typedef lm_id_type const* const_iterator;

  void check_special_id(lm_id_type &special,lm_id_type should,std::string name) {
    if (special!=should) {
      SBMT_WARNING_MSG(lm_domain,"LW LM had wrong id for %1% - got %2%, expected %3%",name%special%should);
      special=should;
    }
    SBMT_TERSE_MSG(lm_domain,"set_ids(%1%)=%2% (LW Vocab constant)",name%special);
  }

  // i'm suspicious that end_id ("</s>" isn't being set properly by normal set_ids ... checking. OK, everything was fine.
  virtual void set_ids() {
    dynamic_ngram_lm::set_ids();
#define CHECKID(s) check_special_id(dynamic_ngram_lm::s,base_ngram_lm<lm_id_type>::s,#s)
    CHECKID(unk_id);
    CHECKID(start_id);
    CHECKID(end_id);
#undef CHECKID
  }


    virtual void set_weights_raw(weight_vector const& weights, feature_dictionary& dict) {}

    virtual const_iterator longest_prefix_raw(const_iterator i,const_iterator end) const
    {
        return concrete().longest_prefix_raw(i,end);
    }
#if 0
    virtual const_iterator longest_suffix_raw(const_iterator i,const_iterator end) const
    {
        return concrete().longest_suffix_raw(i,end);
    }
#endif
    virtual lm_id_type id_raw(const std::string &word,bool add_unk)
    {
        return concrete().id_raw(word);
    }
    virtual std::string const& word_raw(lm_id_type id) const
    {
        return concrete().word_raw(id);
    }
    virtual score_t open_prob_len_raw(const_iterator ctx_and_w,unsigned len) const
    {
        return concrete().open_prob_len_raw(ctx_and_w,len);
    }
    virtual score_t find_bow_raw(const_iterator ctx,const_iterator end) const
    {
        return concrete().find_bow_raw(ctx,end);
    }
    virtual score_t find_prob_raw(const_iterator ctx,const_iterator end) const
    {
        return concrete().find_prob_raw(ctx,end);
    }
    virtual unsigned max_order_raw() const
    {
        return concrete().max_order_raw();
    }
    
    virtual bool set_option(std::string,std::string) {return false; }
  virtual vocab_generator vocab() const;
    lw_ngram_lm(ngram_options const& opt=ngram_options()) : Base("lw",opt) {}

    lw_ngram_lm(ngram_options const& opt,std::istream &i,boost::filesystem::path const& relative_base=boost::filesystem::initial_path()) : Base("lw",opt)
    {
        read(parse_filename(i,relative_base));
    }

    void read(std::string const& fname)
    {
        loaded_filename=fname;
        concrete().read(loaded_filename);
        Base::finish();
    }

    bool loaded() const
    {
        return concrete().loaded();
    }
 protected:

};

}

#endif
