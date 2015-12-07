# ifndef SBMT__NGRAM__NEURAL_LM_HPP
# define SBMT__NGRAM__NEURAL_LM_HPP

# include <sbmt/ngram/dynamic_ngram_lm.hpp>
# include <sbmt/token/indexed_token.hpp>
# include <boost/thread/tss.hpp>
# include <graehl/shared/dynamic_hash_cache.hpp>
# include <graehl/shared/lock_policy.hpp>
# include <nplm/model.h>
# include <nplm/propagator.h>
# include <nplm/vocabulary.h>

class nplm_model {
    boost::shared_ptr<nplm::model> m;
    boost::shared_ptr<nplm::vocabulary> input_vocab, output_vocab;
public:
    boost::int32_t start, end, null;
private:
    char map_digits;
    bool normalize;
    void set_input_vocabulary(const nplm::vocabulary &vocab);
    
    void set_output_vocabulary(const nplm::vocabulary &vocab);
    
    size_t read(const std::string &filename);
    struct prob {
        typedef float result_type;
        nplm_model *lm;
        prob(nplm_model *lm) : lm(lm) {}
        float operator()(boost::uint32_t const* p,unsigned len) const
        {
            return lm->lookup_ngram_uncached(p,len);
        }
    };
    struct div {
        typedef float result_type;
        nplm_model *lm;
        div(nplm_model *lm) : lm(lm) {}
        float operator()(boost::uint32_t const* p,unsigned len) const
        {
            return lm->lookup_ngram_divisor_uncached(p,len);
        }
    };
    prob p;
    div d;
    mutable boost::thread_specific_ptr<nplm::propagator> nlms;
    mutable boost::shared_ptr<graehl::dynamic_hash_cache<prob,graehl::spin_locking> > cache;
    mutable boost::shared_ptr<graehl::dynamic_hash_cache<div,graehl::spin_locking> > dcache;
public:
    nplm::model& model() const { return *m;}

    nplm_model(std::string const& filename, char md = '@', bool normalize = false);

    void set_map_digits(char value);
    
    const nplm::vocabulary& get_input_vocabulary() const;
    
    const nplm::vocabulary& get_output_vocabulary() const;

    int lookup_input_word(const std::string &word) const;

    int lookup_output_word(const std::string &word) const;

    double lookup_ngram(const boost::uint32_t *ngram_a, int n) const;
    double lookup_ngram_uncached(const boost::uint32_t *ngram_a, int n) const;
    double lookup_ngram_divisor_uncached(const boost::uint32_t *ngram_a, int n) const;
    double lookup_ngram( Eigen::Matrix<int,Eigen::Dynamic,1>& ngram
                       , nplm::propagator& prop 
                       ) const;
};

namespace sbmt {


struct neural_lm : public dynamic_ngram_lm
{
    typedef dynamic_ngram_lm Base;

    typedef boost::int32_t word_type;
    typedef double weight_type;
    typedef boost::uint32_t lm_id_type;
    typedef lm_id_type * iterator;
    typedef lm_id_type const* const_iterator;
    word_type start_word;
    
    nplm_model nlmm;
    
    nplm_model const& concrete() const
    {
        return nlmm;
    }
    
    nplm_model& concrete()
    {
        return nlmm;
    }

    virtual void set_weights_raw(weight_vector const& weights,feature_dictionary& dict) 
    {
        //merge_cache();
        //secondary.stats(std::cerr << "secondary:") << '\n';
    }

    inline static score_t score(weight_type w)
    {
        //FIXME: need to convert IMPOSSIBLE to 0?  I think -HUGE_VAL should do it.
        return score_t(w,as_log10());
    }

    neural_lm( ngram_options const& opt, std::istream &i
             , boost::filesystem::path const& relative_base=boost::filesystem::initial_path()
             , bool override_unk=0
             , weight_type p_unk=0.0 )
    : Base("nplm",opt)
    , nlmm(parse_filename(i,relative_base))
    {
        start_word = nlmm.lookup_input_word("<s>");
        Base::finish(); // FIXME: this now does sync_vocab_and_grammar, but before, neural_lm never did.  should be ok.
    }

    virtual lm_id_type id_raw(const std::string &word,bool add_unk)
    {
        //TODO: handle add_unk (wrap ids > max)?
        return nlmm.lookup_input_word(word);
    }
    
    virtual std::string const& word_raw(lm_id_type id) const
    {
        return nlmm.get_input_vocabulary().words().at((word_type)id);
    }
    
    virtual bool set_option(std::string,std::string) { return false; }
    
    score_t uncached_open_prob_len_raw(const_iterator ctx_and_w,unsigned len) const
    {
        shorten_ngram_len(ctx_and_w,len);
        return score(nlmm.lookup_ngram(ctx_and_w,(word_type)len));
    }
    
    score_t open_prob_len_raw(const_iterator ctx_and_w,unsigned len) const
    {
        if ((len < nlmm.model().ngram_size) and (len > 0 and ctx_and_w[0] != start_word)) {
            //std::cerr << "OUT OUT DAMN HYP\n";
            return score(0.0);
        }
        shorten_ngram_len(ctx_and_w,len);
        return uncached_open_prob_len_raw(ctx_and_w,len);
    }
    virtual score_t find_bow_raw(const_iterator ctx,const_iterator end) const
    {
        return score(0.0);
    }
    virtual score_t find_prob_raw(const_iterator ctx,const_iterator end) const
    {
        throw std::runtime_error("find_prob_raw");
        return score(0.0);
    }
    virtual unsigned max_order_raw() const
    {
        return nlmm.model().ngram_size;
    }
    virtual vocab_generator vocab() const
    {
        return vocab_generator(vocab_range_generator(0,nlmm.get_input_vocabulary().size()));
    }
    virtual lm_id_type first_unused_id(unsigned) const 
    {
        return nlmm.get_input_vocabulary().size();
    }

};

}


#endif
