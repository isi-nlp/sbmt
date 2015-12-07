#ifndef SBMT__NGRAM__CLUSTER_LM_HPP
#define SBMT__NGRAM__CLUSTER_LM_HPP

#include <sbmt/ngram/dynamic_ngram_lm.hpp>
#include <sbmt/ngram/big_ngram_lm.hpp>
#include <sbmt/token/indexed_token.hpp>
#include <boost/thread/tss.hpp>
#include <boost/algorithm/string.hpp>
#include <graehl/shared/dynamic_hash_cache.hpp>
#include <graehl/shared/lock_policy.hpp>

namespace sbmt {

struct cluster_lm : public dynamic_ngram_lm
{
    typedef dynamic_ngram_lm Base;

    typedef multi_ngram_lm::weight_type weight_type;
    typedef multi_ngram_lm::lm_id_type lm_id_type;
    typedef multi_ngram_lm::iterator iterator;
    typedef multi_ngram_lm::const_iterator const_iterator;
    
    boost::shared_ptr<multi_ngram_lm> ngrams;
    feature_dictionary fdict;

    virtual void set_weights_raw(weight_vector const& weights,feature_dictionary& dict) {}

    inline static score_t score(weight_type w)
    {
        //FIXME: need to convert IMPOSSIBLE to 0?  I think -HUGE_VAL should do it.
        return score_t(w,as_log10());
    }

    cluster_lm( ngram_options const& opt, std::istream &i
              , boost::filesystem::path const& relative_base=boost::filesystem::initial_path()
              , bool override_unk=0
              , weight_type p_unk=0.0 )
    : Base("cluster",opt)
    {
        std::string filename = parse_filename(i,relative_base);
        std::ifstream ifs(filename.c_str());
        std::string line;
        std::stringstream spec;
        int x = 1;

        spec << "[";
        while (getline(ifs,line)) {
            if (x > 1) spec << ",";
            spec << "clusterlm" << x << "=big[o][" <<  line << "]";
            ++x;
        }
        spec << "]";
        ngrams.reset(new multi_ngram_lm(opt,spec,filename));
        Base::finish(); // FIXME: this now does sync_vocab_and_grammar, but before, never did.  should be ok.
    }

    virtual lm_id_type id_raw(const std::string &word,bool add_unk)
    {
        return ngrams->id_raw(word,add_unk);
    }
    virtual std::string const& word_raw(lm_id_type id) const
    {
        return ngrams->word_raw(id);
    }
    virtual void set_ids() { ngrams->set_ids(); }    
    virtual bool set_option(std::string name,std::string val) 
    { 
        if (name == "cluster-weights") {
            std::vector<std::string> wstr;
            boost::split(wstr,val,boost::is_any_of(","));
            std::stringstream sstr;
            int x = 1;
            BOOST_FOREACH(std::string wv, wstr) {
                if (x > 1) sstr << ",";
                sstr << "clusterlm" << x << ":" << wv;
                ++x;
            }
            weight_vector weights;
            read(weights,sstr.str(),fdict);
            ngrams->set_weights_raw(weights,fdict);
            return true;
        } else {
            return false;
        }
    }
    
    score_t open_prob_len_raw(const_iterator ctx_and_w,unsigned len) const
    {
        return ngrams->open_prob_len_raw(ctx_and_w,len);
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
        return ngrams->max_order_raw();
    }
    virtual vocab_generator vocab() const
    {
        return ngrams->vocab();
    }
    virtual lm_id_type first_unused_id(unsigned c) const 
    {
        return ngrams->first_unused_id(c);
    }

};

}


#endif
