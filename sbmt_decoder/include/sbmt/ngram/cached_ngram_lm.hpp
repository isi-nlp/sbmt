#ifndef SBMT_NGRAM__CACHED_DYNAMIC_NGRAM_LM_HPP
#define SBMT_NGRAM__CACHED_DYNAMIC_NGRAM_LM_HPP

#include <graehl/shared/dynamic_hash_cache.hpp>
#include <graehl/shared/lock_policy.hpp>
#include <iostream>
#include <sbmt/ngram/base_ngram_lm.hpp>
#include <boost/cstdint.hpp>
namespace sbmt {

template <class Impl,class Locking=graehl::spin_locking>
struct cached_ngram_lm : public Impl
{
    typedef Impl impl_type;
    typedef cached_ngram_lm<impl_type> self_type;
    typedef boost::uint32_t const* const_iterator;
private:
    Impl& base()
    {
        return static_cast<Impl&>(*this);
    }
    score_t uncached_open_prob_len_raw(const_iterator ctx_and_w,unsigned len) const
    {
        return impl_type::open_prob_len_raw(ctx_and_w,len);
    }
    friend class lm_prob;
    struct lm_prob
    {
        typedef score_t result_type;
        self_type *lm;
        lm_prob(self_type *lm) : lm(lm) {}
        score_t operator()(const_iterator p,unsigned len) const
        {
            return lm->uncached_open_prob_len_raw(p,len);
        }

    };
    lm_prob p;

    graehl::dynamic_hash_cache<lm_prob,Locking> cached_p;

 public:

    cached_ngram_lm(ngram_options const& opt,std::istream &i,boost::filesystem::path const& relative_base,unsigned cache_size=10000)
        :
        impl_type(opt,i,relative_base)
        ,p(this) // \todo source of compiler warning in msvc8: initializing member from "this", but "this" is not finished initializing...
        ,cached_p(p,impl_type::max_order_raw(),cache_size)
    {}

    void stats(std::ostream &o) const
    {
        cached_p.stats(o);
    }

    score_t open_prob_len_raw(const_iterator ctx_and_w,unsigned len) const
    {
        impl_type::shorten_ngram_len(ctx_and_w,len);
        return cached_p(ctx_and_w,len);
    }


    virtual void set_weights_raw(weight_vector const& weights, feature_dictionary& dict)
    {
        Impl::set_weights_raw(weights,dict);
        cached_p.flush(); // new weights make old values wrong w.r.t. multi-lm weights
        SBMT_TERSE_MSG(lm_domain,"flushed cache for lm %1%",impl_type::description());
    }
    
    //virtual bool set_option(std::string s,std::string t) {return base().set_option(s,t); }

};



#if 0
struct wrapping_cached_dynamic_ngram_lm : public dynamic_ngram_lm
{
    ngram_ptr lm;
    struct lm_prob
    {
        typedef score_t result_type;
        dynamic_ngram_lm *lm;
        lm_prob(dynamic_ngram_lm *lm) : lm(lm) {}
        void get(unsigned *p,unsigned len,result_type *r) const
        {
            *r=lm->open_prob_len_raw(p,len);
        }

    };
    lm_prob p;

    graehl::dynamic_hash_cache<score_t,lm_prob> cached_p;

    cached_dynamic_ngram_lm(ngram_ptr lm,unsigned cache_size=10000)
        :lm(lm)
        ,p(lm.get())
        ,cached_p(p,lm->max_order_raw(),cache_size)
    {}

    score_t open_prob_len_raw(const_iterator ctx_and_w,unsigned len) const
    {
        shorten_ngram_len(ctx_and_w,len);
        return cached_p(ctx_and_w,len);
    }

    void set_weights_raw(score_combiner const& combine)
    {
        lm->set_weights_raw(combine);
        cached_p.flush();
    }

//    void update_token_factory(tf)

    // forward everything!
};
#endif


}



#endif
