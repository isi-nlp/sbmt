#ifndef SBMT__NGRAM__MULTI_NGRAM_LM_HPP
#define SBMT__NGRAM__MULTI_NGRAM_LM_HPP

//TODO: call recursive _raw versions for less logging checks?

#ifdef _MSC_VER
# include <malloc.h>
#endif

#include <graehl/shared/assoc_container.hpp>
#include <graehl/shared/maybe_update_bound.hpp>
#include <graehl/shared/word_spacer.hpp>
#include <sbmt/ngram/dynamic_ngram_lm.hpp>
#include <sbmt/token/in_memory_token_storage.hpp>
namespace sbmt {

struct multi_ngram_lm : public dynamic_ngram_lm
{
    typedef dynamic_ngram_lm Base;
    typedef Base::Fat Fat;
    typedef in_memory_token_storage tf_type;

    typedef dynamic_ngram_lm single_lm;
    typedef boost::shared_ptr<single_lm> lm_p;
    typedef std::vector<lm_p> lms_type;
    typedef std::vector<lm_id_type> ids_type;

    struct indexed_token_map
    {
        lm_id_type id_or_unk;
        ids_type ids;
    };


    virtual unsigned n_components() const
    {
        unsigned n = 0;
        for (size_t x = 0; x != lms.size(); ++x) n += lms[x]->n_components();
        return n;
    }

    typedef std::vector<indexed_token_map> ids_per_id_type;
    typedef std::vector<std::string> lm_names_type;

    struct interpolator
    {
        typedef multi_ngram_lm::weight_type weight_type;
        typedef std::vector<weight_type> weights_type; // one per lm
        weights_type w;
//        bool log_linear;
        BOOST_STATIC_CONSTANT(bool,log_linear=true);
        interpolator() {}
        interpolator(lms_type const& lms,weight_vector const& weights,feature_dictionary& dict, bool log_linear=true)
        {
            init(lms,weights,dict,log_linear);
        }
        static inline void assert_log_linear(bool log_lin)
        {
            if (!log_lin)
                throw std::runtime_error("multi_ngram_lm::interpolator: only loglinear interpolation is supported due to difficulty handling delayed backoffs properly");
        }

        void assert_log_linear() const
        {
            assert_log_linear(log_linear);
        }


        void init(lms_type const& lms, weight_vector const& weights, feature_dictionary& dict, bool log_lin=true)
        {
            w.resize(lms.size());
            assert_log_linear(log_lin);
            for (unsigned l=0;l<lms.size();++l)
                w[l]=lms[l]->set_weights(weights,dict,0); // no feature for child lm weight -> don't use child lm at all
        }
        score_t identity() const
        {
            if (log_linear)
                return as_one();
            else
                return as_zero();
        }
        score_t scale(score_t prob,weight_type w) const
        {
            return prob;
            //return log_linear ? pow(prob,w) : prob*w;
        }
        score_t scale_i(score_t prob,unsigned i) const
        {
            return scale(prob,w[i]);
        }
        score_t accumulate(score_t total,score_t scaled_prob) const
        {
            return log_linear ? total*scaled_prob : total+scaled_prob;
        }
        score_t accumulate_scaled(score_t total,score_t prob,weight_type w) const
        {
            return accumulate(total,scale(prob,w));
        }
        score_t accumulate_scaled_i(score_t total,score_t prob,unsigned i) const
        {
            return accumulate(total,scale_i(prob,i));
        }
    };

    virtual void set_weights_raw(weight_vector const& weights, feature_dictionary& dict)
    {
        interp.init(lms,weights,dict);
        Base::set_weights_raw(weights,dict);
        Base::own_weight = 1.0;
    }
    
    virtual bool set_option(std::string s1,std::string s2) 
    { 
        bool k = false; 
        for (unsigned l=0,n=n_lms();l<n;++l) {
            k = k or lms[l]->set_option(s1,s2);
        }
        return k;
    }

    interpolator interp;

    lms_type lms;

    tf_type tf;
  virtual vocab_generator vocab() const {
    return vocab_generator(vocab_range_generator(tf.ibegin(),tf.iend()));
  }
  virtual lm_id_type first_unused_id(unsigned) const {
    return tf.iend();
  }

    ids_per_id_type ids_per;

    multi_ngram_lm(ngram_options const& opt=ngram_options())
    : Base("multi",opt) {}

    virtual dynamic_ngram_lm *lm_named(std::string const& named)
    {
        for (unsigned l=0,nlms=n_lms();l<nlms;++l) {
            dynamic_ngram_lm *r=lms[l]->lm_named(named);
            if (r) return r;
        }
        return NULL;
    }

    virtual void set_ids() /// you call this after your constructor (multiple calls are ok; constructor can do it for you).  not done automatically because id_raw can't be called until you init your vocab.  may be done after loading lm.
    {
        for (unsigned l=0,nlms=n_lms();l<nlms;++l) {
            lms[l]->set_ids(); // necessary for biglm so you can detect unk words properly using id_raw->maybe_unk_id which uses child lm unk_id
        }
        ids_per.clear();
        init_unk_id("<unk>");
        start_id=id_raw("<s>",false);
        end_id=id_raw("</s>",false);
//        for (size_t xl = 0; xl != lms.size(); ++xl) {
//            vocab_generator vgen = lms[xl]->vocab();
//            while (vgen) {
//                id_raw(lms[xl]->word_raw(vgen()),false);
//            }
//        }
    }

    unsigned n_lms() const { return lms.size(); }

    // note: you may not use this to seed <unk> in the first place.  use update_token_factory before calling id_raw and all is well
    virtual lm_id_type id_raw(const std::string &word,bool add_unk)
    {
        lm_id_type i = tf.get_index(word);
        return add_unk ? never_unk_id(i,word) : maybe_unk_id(i,word);
    }

    virtual std::string const& word_raw(lm_id_type id) const
    {
        return tf.get_token(id);
    }

    virtual unsigned max_order_raw() const
    {
        unsigned m=0;
        for (lms_type::const_iterator l=lms.begin(),le=lms.end();l!=le;++l)
            graehl::maybe_increase_max(m,(*l)->max_order_raw());
        return m;
    }

//FIXME: use thread local storage when variable sized stack arrays not allowed:
//boost::scoped_array<lm_id_type> scratch=new lm_id_type[... - then can use proper encapsulation for this:
#ifdef _MSC_VER
# define MULTI_NGRAM_ARRAY(name,size) lm_id_type *name=(lm_id_type*)_alloca(size*sizeof(lm_id_type))
    /* The alloca() function allocates space in the stack frame of the caller,
     * and returns a pointer to the allocated block. This temporary space is
     * automatically freed when the function from which alloca() is called
     * returns. */
#else
# define MULTI_NGRAM_ARRAY(name,size) lm_id_type name[size]
#endif
//to be clear: both of the above should be threadsafe since they use stack.  no bug reports either.

    /* MULTI_NGRAM_DO:
       l: lm[l]
       [beg... beg+len): source multi lm ids (= native token indexes)
       [i..end) per lm[l], child lm ids
       x: expression in terms of l,i,end
     */
#define MULTI_NGRAM_DO(l,beg,len,i,end,x) do { \
        MULTI_NGRAM_ARRAY(multi_ngram_do_scratch,n_lms()*len); \
    scratch_transcribe(multi_ngram_do_scratch,beg,len);  \
    for (unsigned l=0,nlms=n_lms();l<nlms;++l) {                    \
            const_iterator i=multi_ngram_do_scratch+(len*l),end=i+len; \
            x                                           \
                } } while(0)

    virtual void print_further_sequence_details(std::ostream &o
                                              ,const_iterator history_start
                                              ,const_iterator score_start
                                              ,const_iterator end
                                                , bool factors=false
        ) const
    {
        unsigned skip=score_start-history_start;
        unsigned len=end-history_start;
        if (factors) {
            o << ' ' << name << "-sentence={{{";
            print(o,history_start,end,' ');
            o << "}}}";
        }

        MULTI_NGRAM_DO(l,history_start,len,a,b,
                       lms[l]->print_sequence_details_raw(o,a,a+skip,b,factors););
    }

    /// may override: returns r such that [b,r) is the longest known history (i.e. length maxorder-1 max)
    virtual const_iterator longest_prefix_raw(const_iterator i,const_iterator end) const
    {
        assert_short_history(i,end);
        int ret=0;
        unsigned len=end-i;
        MULTI_NGRAM_DO(l,i,len,a,b,{
                graehl::maybe_increase_max(ret,lms[l]->longest_prefix(a,b)-a);
            });
        return i+ret;
    }

    /// may override: reverse of longest_prefix (returns r such that [r,end) is the longest seen history)
    virtual const_iterator longest_suffix_raw(const_iterator i,const_iterator end) const
    {
        assert_short_history(i,end);
        int ret=0;
        unsigned len=end-i;
        MULTI_NGRAM_DO(l,i,len,a,b,{
                graehl::maybe_increase_max(ret,b-lms[l]->longest_suffix(a,b));
            });
        return end-ret;
    }

    score_t open_prob_len_raw(const_iterator ctx_and_w,unsigned len) const
    {
        score_t ret=interp.identity();
        MULTI_NGRAM_DO(l,ctx_and_w,len,a,b,{
                ret=interp.accumulate_scaled_i(ret,lms[l]->prob(a,b),l);
            });
        return ret;
    }

    // only makes sense for loglinear interp. otherwise you need to track and combine per-lm residues
    score_t find_bow_raw(const_iterator ctx,const_iterator end) const
    {
        interp.assert_log_linear();
        score_t ret=interp.identity();
        bool any=false;
        unsigned len=end-ctx;
        MULTI_NGRAM_DO(l,ctx,len,a,b,{
                       score_t bow=lms[l]->find_bow(a,b);
                       if (!bow.is_zero()) {
                           any=true;
                           ret=interp.accumulate_scaled_i(ret,bow,l);
                       }
            });
        return any ? ret : as_zero();
    }

    ///FIXME: probably doesn't work as intended?  will give false complaints about wrong LM score
    score_t find_prob_raw(const_iterator ctx,const_iterator end) const
    {
        interp.assert_log_linear();
        score_t ret=interp.identity();
        bool any=false;
        unsigned len=end-ctx;
        MULTI_NGRAM_DO(l,ctx,len,a,b,{
                       score_t prob=lms[l]->find_prob(a,b);
                       if (!prob.is_zero()) {
                           any=true;
                           ret=interp.accumulate_scaled_i(ret,prob,l);
                       }
            });
        return any ? ret : as_zero();
    }

    virtual void describe(std::ostream &o) const {
        o << name << "=multi";
        o << this->opt;
        o << '[';
        graehl::word_spacer sep(',');
        for (unsigned l=0,n=n_lms();l<n;++l) {
            o << sep;
            lms[l]->describe(o);
        }
        o << ']';
    }

    void clear()
    {
        ids_per.clear();
        lms.clear();
    }

    void read_from_spec(std::istream &in,boost::filesystem::path const& relative_base=boost::filesystem::initial_path())
    {
        clear();
        loaded_filename="multi-lm-spec";
        char c;
        if (in>> c && c=='[') {
            while(in>>c) {
                if (c==',') {continue;}
                if (c==']') {
                    Base::finish();
                    return;
                }
                in.unget();
                lms.push_back(ngram_lm_factory::instance().create(in,relative_base));
            }
        }
        throw std::runtime_error("expected multi[OPTIONS][lm1=LMSPEC1,lm2=LMSPEC2], where LMSPEC1 is e.g. lw[OPTIONS][filename], and [OPTIONS] is e.g. [@o] or [c]");
    }

    multi_ngram_lm(ngram_options const& opt_,std::istream &in,boost::filesystem::path const& relative_base=boost::filesystem::initial_path()) : Base("multi",opt_)
    {
        if (!opt.is_open()) {
            SBMT_INFO_STREAM(lm_domain,"Requested multi-lm with options "<<opt<<"; forcing to open-class (use child LM options instead, except for @ (digit->@))");
            opt.set_open(); // otherwise child lms would be confused
            SBMT_INFO_STREAM(lm_domain,"Using modified multi-lm options "<<opt);
        }
        read_from_spec(in,relative_base);

    }

    virtual feature_t* sequence_prob_array(feature_t* v,const_iterator seq,const_iterator i,const_iterator end) const
    {
        //v=Fat::sequence_prob_array(v,seq,i,end);
        unsigned len=end-seq,skip=i-seq;
        MULTI_NGRAM_DO(l,seq,len,a,b,
                       v=lms[l]->sequence_prob_array(v,a,a+skip,b););
        return v;
    }

    virtual feature_t* bow_interval_array(feature_t* v,const_iterator seq,const_iterator i,const_iterator end) const
    {
        //v=Fat::bow_interval_array(v,seq,i,end);
        unsigned len=end-seq,skip=i-seq;
        MULTI_NGRAM_DO(l,seq,len,a,b,
                       v=lms[l]->bow_interval_array(v,a,a+skip,b););
        return v;
    }

 protected:
    /// [b...b+len),[b+len...b+2*len), ... [b+(L-1)*len,b+L*len) contains translations of [i,end) for each of the L=n_lms() lms: b[l*len+p] = lm[l]->id(i[p]), for p in [0,len)
    unsigned scratch_transcribe(iterator b,const_iterator i,unsigned len) const
    {
        for (const_iterator end=i+len;i!=end;++i,++b) {
            ids_type const& ids=ids_per[*i].ids;
            for (unsigned l=0,n=n_lms();l<n;++l)
                b[l*len]=ids[l];
        }
        return len;
    }

    lm_id_type never_unk_id(lm_id_type i,const std::string &word)
    {
        indexed_token_map &m=graehl::at_expand(ids_per,i);
        if (m.ids.empty()) {
            m.id_or_unk=i;
            unsigned l=0,n=n_lms();
            m.ids.resize(n);
            for (;l<n;++l)
                m.ids[l]=lms[l]->id(word,false);
        }
        return i;
    }

    // note: if word is unknown to all child lms, then we return the <unk> token index (from tf)
    lm_id_type maybe_unk_id(lm_id_type i,const std::string &word)
    {
        indexed_token_map &m=graehl::at_expand(ids_per,i);
        lm_id_type &ret=m.id_or_unk;
        if (m.ids.empty()) {
            ret=unk_id;
            unsigned l=0,n=n_lms();
            m.ids.resize(n);
            for (;l<n;++l)
                if ((m.ids[l]=lms[l]->id(word,false))!=lms[l]->unk_id)
                    ret=i;
        }
        return ret;
    }

    void init_unk_id(std::string const& unk_string="<unk>")
    {
        unk_id = tf.get_index(unk_string);
        indexed_token_map &m=graehl::at_expand(ids_per,unk_id);
        m.ids.resize(n_lms());
        for (unsigned l=0,n=n_lms();l<n;++l) {
            m.ids[l]=lms[l]->id(unk_string,false); //FIXME: should be lms[l]->unk_id ??? assert?
            assert(m.ids[l]==lms[l]->unk_id);
        }
    }


};



}

#endif
