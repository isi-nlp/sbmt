#ifndef SBMT_NGRAM__base_ngram_lm_hpp
#define SBMT_NGRAM__base_ngram_lm_hpp

//#define BASE_NGRAM_ALWAYS_SKIP_TM_UNKNOWN
// always skip @UNKNOWN@ (without inspecting lmstring).  not needed since derivation.hpp:collect_english_lmstring works now, and uses binary lmstring to build sentence from derivation.


#define BASE_NGRAM_BOW_WITH_HOLES
// david trains LMs with abc having backoff, bc not having it, c having it.  this sucks, but we can just query all by defining BASE_NGRAM_BOW_WITH_HOLES

//#define NGRAM_FACTORS_VERIFY_EPSILON 1e-4
// if NGRAM_FACTORS_VERIFY_EPSILON is defined, then we print out details about bow and prob entries used in computing nbest sentence-at-once score, and complain if the total doesn't match

//TODO: returning length of maximal ngram when p(w|context) (without additional
//lookups) might be a nice thing, e.g. fancy LM combination other than
//f(p1,p2...)

//#define DEBUG_LM_SHORTEN_HISTORY
#include <vector>
#include <sbmt/logmath.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>
#include <boost/config.hpp>
#include <Shared/Common/Vocab/Vocab.h>
#include <sbmt/logging.hpp>
#include <graehl/shared/maybe_update_bound.hpp>
#include <graehl/shared/word_spacer.hpp>
#include <graehl/shared/assoc_container.hpp>
#include <sbmt/feature/feature_vector.hpp>
#include <sbmt/edge/component_scores.hpp>
#include <gusc/string/escape_string.hpp>
#include <boost/locale.hpp>
#define STAT_LM

#ifdef DEBUG_LM
# include <iostream>
# define DEBUG_LM_OUT std::cerr
#endif
namespace sbmt {

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(ngram_domain,"ngram",root_domain);
// why 2 domains? in case you want a separate file. ngram scores only go in ngram; lm has configuration information, not individual scores
//SBMT_SET_DOMAIN_LOGFILE(ngram_domain,"ngram.log.gz");
//SBMT_SET_DOMAIN_LOGGING_LEVEL(ngram_domain,verbose);

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(lm_domain,"lm",root_domain);


struct ngram_options
{
    //FIXME: default to open?
    // unknown_word_prob defaults to 1.0, because there can be no implicit features in decoding.
    // if you are going to use a closed model (training unk), either specify lm-unk,
    // or the feature weight defaults to 0 just like any other feature. this is critical to
    // tuning systems like mira --michael
    ngram_options( bool lm_at_numclass = true
                 , bool openclass_lm = true
                 , score_t unknown_word_prob = 1.0
                 , bool extra_open_unk_prob = false )
        : lm_at_numclass(lm_at_numclass)
        , openclass_lm(openclass_lm)
        , unknown_word_prob(unknown_word_prob)
        , check_unks(!openclass_lm || extra_open_unk_prob)
    {}

    void check() const
    {
        if (is_closed() && !check_unks)
            throw std::runtime_error("Unsupported ngram_options: closed LM with check_unks=false)");
    }

    template <class O>
    void print(O &o,bool include_unk_weight=false,double unk_weight_scale=1) const
    {
        check();
//        graehl::word_spacer comma(',');
        o << "[";
        if (lm_at_numclass)
            o << "@";
        if (is_open())
            o << 'o';
        else if (is_closed())
            o << 'c';
        else {
            assert(is_open_plus_unk());
            o << 'u';
        }
        if (include_unk_weight && check_unks && !unknown_word_prob.is_one()) // != score_t(as_one()))
            o << '='<<unknown_word_prob*unk_weight_scale;
        o << "]";
    }

    char const* unk_prob_desc() const
    {
        check();
        if (is_open_plus_unk()) return "additional (open-class)";
        if (is_open()) return "unused (open-class)";
        if (is_closed()) return "(closed-class)";
        assert(0);
        return ""; // check() prevents this
    }

    // returns true if ok  (expect to see either ] or =score)
    template <class I>
    bool read_optional_p_unk(I &i)
    {
        char c;
        if (!(i>>c))
            return false;
        if (c=='=') {
            if (!(i >> unknown_word_prob))
                return false;
            else
                i.unget();
        }
        return true;
    }

    bool is_open_plus_unk() const
    {
        return openclass_lm && check_unks;
    }

    bool is_open() const
    {
        return openclass_lm && !check_unks;
    }

    bool is_closed() const
    {
        return !openclass_lm;
    }


    void set_open()
    {
        openclass_lm=true;
        check_unks=false;
    }

    void set_open_plus_unk()
    {
        openclass_lm=true;
        check_unks=true;
    }

    void set_closed()
    {
        openclass_lm=false;
        check_unks=true;
    }

    template <class I>
    void read(I &i)
    {
        set_defaults();
        set_closed();
        lm_at_numclass=false;
        char c;
        if (!(i >> c)) return;
        if (c!='[') {
            i.unget();
            return;
        }
        while (i >> c) {
            switch(c) {
            case ']': return;
            case ',': break;
            case '@': lm_at_numclass=true;break;
            case 'c' : set_closed();break;
            case 'o' : set_open();break;
            case 'u' : set_open_plus_unk();break;
            case '=': if (!(i >> unknown_word_prob)) goto badform;
                break;
            default:
                goto badform;
            }
        }
        badform:
        throw std::runtime_error("Couldn't parse ngram_options.  Valid examples: \"[@,c=10^-8]\",\"[]\",\"[u]\",\"[@o]\"");
    }

    friend std::ostream& operator << (std::ostream &o,ngram_options const &n)
    { n.print(o);return o;}

    friend std::istream& operator >> (std::istream &i,ngram_options &n)
    { n.read(i);return i;}

    bool lm_at_numclass;
    bool openclass_lm;
    score_t unknown_word_prob;
    bool check_unks;

    void set_defaults()
    {
        *this=ngram_options();
    }

};


/// utf8 means we can handle ASCII digits byte by byte
inline void replace_digits_with(std::string &s,char c='@')
{
    for(std::string::iterator i=s.begin(),e=s.end();i!=e;++i)
        if (std::isdigit(*i))
            *i =  c;
}

//FIXME: whether closed or open, always report # of unks when scoring, so separate feature can be used (trained).  in case of open, additionally use p(<unk>) from LM for open, getting context-dependent backoff.
template <class LMWord>
struct base_ngram_lm
{
    typedef LMWord lm_id_type;

    typedef lm_id_type * iterator;
    typedef lm_id_type const* const_iterator;
    typedef std::pair<boost::uint32_t,score_t> feature_t;

    enum {
        unk_id = LW::LWVocab::UNKNOWN_WORD,
        start_id = LW::LWVocab::START_SENTENCE,
        end_id = LW::LWVocab::END_SENTENCE,
        null_id = LW::LWVocab::NONE,
        invalid_id = LW::LWVocab::INVALID_WORD,
        bo_id = LW::LWVocab::INVALID_WORD, // intended to use in last slot of left context, for prematurely shortened but needing more backoffs
        first_valid_id = LW::LWVocab::FIRST_VALID_WORD_ID
    };

    /*
    static inline bool is_null(lm_id_type id)
    {
        return id==null_word_id;
    }

     /// excludes start, end sentence, and unknown
    inline static bool normal_word(lm_id_type i)
    {
        return i >= first_valid_word_id;
    }
    /// includes everything but null and bo/invalid markers
    inline static bool some_word(lm_id_type i)
    {
        return i != null_word_id && i!= bo_word_id;
    }
    */


    void no_imp(const char *whine="base_ngram_lm: no implementation provided (spank a programmer)!")
    {
        throw std::logic_error(whine);
    }

    void stats(std::ostream &o) const {
    }

    /// [history_start,score_start,end): \prod{s in [score_start,end)} p(s|history_start...s-1)
    void print_further_sequence_details(std::ostream &o,const_iterator history_start,const_iterator score_start,const_iterator end,bool factors=false) const {}

//// must provide this yourself; "ambiguous" to compiler if I supply a default here?
//    void set_weights_raw(score_combiner const& combine) {}

    /// all of the following methods *must* be defined:
#if 0
    // hidden from compiler because you get multiple inheritance "ambiguity" otherwise

    /// override (don't worry about digit->@ replacement, it's already done
    lm_id_type id_raw(const std::string &tok) { no_imp();return 0;}

    /// override:
    std::string const& word_raw(lm_id_type id) const  { no_imp();return ""; }

    /// NOTE: LM must handle request for contexts longer than the max order (i.e. adjust the bounds if necessary)
    /// just need one of:
    score_t open_prob_raw(const_iterator ctx,const_iterator end) const {no_imp();return 0;}
    score_t open_prob_len_raw(const_iterator ctx_and_w,unsigned len) const {no_imp();return 0;}

    /// (for up to max_order-1 - grams, return score_t(0) if phrase is unknown, bow otherwise.  i.e. do not call for max_order or higher or program fails (this is opposite the open_prob call which protects against contexts)
    score_t find_bow_raw(const_iterator ctx,const_iterator end) const {no_imp();return 0;}

    unsigned max_order_raw() const {no_imp();return 0;}

    template <class GT> void reset_table(GT& gram) {no_imp();}

    /// override; should be called only after reset_table(g):
    template<class Gram>
        lm_id_type get_lm_id(indexed_token const& tok, Gram const& g) const { no_imp();return 0; }

    bool loaded() const  { no_imp();return 0; }
#endif
};

std::string parse_filename(std::istream &i,boost::filesystem::path const& relative_base=boost::filesystem::initial_path()); // consume [filename] or ["filename with ] [ \\ \""], returning filename.  relative paths are appended to relative_base
void write_filename(std::ostream &o,std::string const& filename);

// usage: augmented_lm : public base_ngram_lm<unsigned>,fat_ngram_lm<augmented_lm>
template <class CRTP,class LMWord>
struct fat_ngram_lm
{
 protected:
    boost::locale::generator gen;
    std::locale loc;// = gen("en_US.utf-8");
    typedef CRTP C;
    typedef fat_ngram_lm<CRTP,LMWord> self_type;
 public:
    typedef std::pair<boost::uint32_t,score_t> feature_t;
    typedef double weight_type;
    typedef LMWord lm_id_type;
    typedef lm_id_type * iterator;
    typedef lm_id_type const* const_iterator;

    std::string type;
    std::string name; // used for printing, finding loglinear interp. weights
    boost::uint32_t nameid;
    boost::uint32_t nameunkid;
    std::string loaded_filename;


    int max_order_cached;
    int max_order() const
    {
        return max_order_cached;
    }

    void describe(std::ostream &o) const {
        o << name << '='<<type;
        this->opt.print(o,true,own_weight);
        write_filename(o,loaded_filename);
    }
    std::string description() const
    {
        std::stringstream ss;
        sub().describe(ss);
        return ss.str();
    }


    /// will search multi-lm.  use case: clm to retrieve left/right mixed e/f lms
    virtual CRTP *lm_named(std::string const& named)
    {
        return name==named ? &sub() : NULL;
    }

/* COMPONENT SCORE SUPPORT */
    virtual unsigned n_components() const
    {
        return opt.is_open() ? 1 : 2;
    }

    virtual ~fat_ngram_lm() {}

    inline static std::string factor_feature_for(std::string const& base)
    {
        return base+"-factors";
    }

    std::string unk_feature_for(std::string const& base) const
    {
        if (opt.is_open())
            return base+"-open-unk"; // so external dotproduct verifiers don't get confused by an unused weight for lm-unk
        else
            return base+"-unk";
    }

    inline static std::string combined_feature_for(std::string const& base)
    {
        return base+"-with-unk";
    }


  void update_unk_prob(double unkcost)
  {
    score_t newunk(unkcost,as_neglog10());
    if (opt.unknown_word_prob!=newunk) {
      opt.unknown_word_prob=newunk;
      SBMT_INFO_STREAM(lm_domain,"From weight "<<unk_feature_for(name)<<"="<<unkcost<<", "<<opt.unk_prob_desc()<<" p_"<<name<<"(<unk>)="<<opt.unknown_word_prob);
    }
  }

    void update_unk_prob(weight_vector const& weights, feature_dictionary& dict)
    {
        std::string unkname=unk_feature_for(name);
        nameunkid = dict.get_index(unkname);
        double unkwt=get(weights,dict,unkname);
        //if (unkwt) // weird tuning if closed lm and unkwt 0 used - will be default p(unk) instead!
        update_unk_prob(unkwt); //always update. avoids bug where weight is set to nonzero then back to zero cost.
    }

    weight_type own_weight; // note: this doesn't modify the result of any prob/bo queries.  you can use it yourself (that means you, ngram_info, now that info_factory always has info_wt=1). exception: prob(...) applies own_weight. and unknown_prob separately (closed lm w/ 2 separate features)

    void log_weight(std::string const& feat)
    {
        SBMT_INFO_STREAM(lm_domain,"New weight for LM "<<name<<" from feature weight "<<feat<<'='<<own_weight);
    }

    bool is_multi() const
    {
        return type == "multi";
    }


    weight_type set_weights(weight_vector const& weights, feature_dictionary& dict, weight_type default_weight=0.0) {
        std::string const& combined_feat=combined_feature_for(name);
        double wcf = get(weights,dict,combined_feat);
        if (wcf != 0.0) {
            own_weight=wcf;
            nameid=dict.get_index(combined_feat);
            log_weight(combined_feat);
            update_unk_prob(weights,dict);
        } else {
            double wn = get(weights,dict,name);
            nameid=dict.get_index(name);
            if (wn != 0.0) {
                own_weight=wn;
                log_weight(name);
            } else {
                if (!is_multi())
                    SBMT_TERSE_STREAM(lm_domain,"No feature weight found for LM "<<name<<"; using default: "<< default_weight);
                own_weight=default_weight; // default should not be anything else, or it screws up tuning --michael.
                // fine; see argument default_weight
            }
            update_unk_prob(weights,dict);
        }
        sub().set_weights_raw(weights,dict);
        SBMT_INFO_STREAM(lm_domain,"Using weight for "<<name<<"="<<own_weight);
        return own_weight;
    }

  void set_weight_1(double unkcost=10) {
    own_weight=1;
    SBMT_INFO_STREAM(lm_domain,"forcing weight for ngram LM "<<name<<"=1");
    update_unk_prob(unkcost);
  }

    void set_ids() {}

    // must call this before bow_interval!!!
    void cache_max_order()
    {
        set_max_order(sub().max_order_raw());
    }
    void set_max_order(int max)
    {
        max_order_cached=max;
    }

    /*
    struct lm_ids : public std::vector<lm_id_type>
    {
        typedef std::vector<lm_id_type> Base;

        lm_id_type * begin()
        { return &Base::front(); }
        lm_id_type const* begin() const
        { return &Base::front(); }
        lm_id_type * end()
        { return &Base::back()+1; }
        lm_id_type const* end() const
        { return &Base::back()+1; }

        template <class indexed_tokens_it>
        lm_ids(self_type const& lm,indexed_tokens_it i,indexed_tokens_it end,bool whole_sentence=true)
            : Base((whole_sentence?2:0)+(end-i))
        {
            lm_id_type *o=begin();
            if (whole_sentence) *o++ = lm.sub().start_id;
            for (;i!=end;++i) {
                indexed_token t=*i;
#ifdef BASE_NGRAM_ALWAYS_SKIP_TM_UNKNOWN
                if (t==lm.tm_unk) {
                    Base::pop_back(); // ugly: just use push_back throughout?
                    continue;
                }
#endif
                *o++=lm.get_lm_id(t);
            }

            if (whole_sentence) *o = lm.sub().end_id;
        }

    };


    friend struct lm_ids;
    */

    /// [history_start,score_start,end): \prod{s in [score_start,end)} p(s|history_start...s-1)
    score_t print_sequence_details_raw(std::ostream &o,const_iterator history_start,const_iterator score_start,const_iterator end,bool factors=false) const
    {
        SBMT_DEBUG_EXPR(ngram_domain,
                          {
                              continue_log(str)<<"print_sequence_details:";
                          });
        score_t p_all=this->sequence_prob(history_start,score_start,end);
        unsigned n_unk=this->count_unk(score_start,end);
        score_t feat_unk(n_unk,as_neglog10()); // allow printing as 10^-n if not neglog10
        score_t p_just_lm=p_all;
        if (!opt.is_open()) {
            score_t p_unk(opt.unknown_word_prob,n_unk);
            p_just_lm/=p_unk;
            o << ' ' << combined_feature_for(name)<<'='<<p_all;
        }

#ifdef NGRAM_FACTORS_VERIFY_EPSILON
        factors=true;
#endif
        if (factors && !is_multi()) { // individual bow are not meaningful for multi!
            o << ' ' << factor_feature_for(name) << "={{{";
            score_t p_just_lm_2=print_factors_raw(o, history_start, score_start, end);
            o << "=}}}";
#if defined(NGRAM_FACTORS_VERIFY_EPSILON)
            if (!graehl::very_close(p_just_lm.log(),p_just_lm_2.log(),NGRAM_FACTORS_VERIFY_EPSILON)) {
                o << ' ' << factor_feature_for(name)<<"-mismatch="<<p_just_lm.relative_difference(p_just_lm_2);
            SBMT_WARNING_STREAM(lm_domain,"Computed nbest ngram score differs between regular probability and decomposition via bow/prob lookup.  For LM: "<<name<<" regular="<<p_just_lm<<" factored="<<p_just_lm_2);
            }
#endif
        }
        o << ' ' << name << '='<<p_just_lm;
        o << ' ' << unk_feature_for(name)<<'='<<feat_unk;
        sub().print_further_sequence_details(o,history_start,score_start,end,factors);
        return p_all;
    }

    // print (and return) p(w|history)*pbo1*pbo2...
    score_t print_factor(std::ostream &o,const_iterator beg,const_iterator w) const
    {
        const_iterator e=w+1;
        const_iterator i=e-max_order();
        if (i<beg)
            i=beg;
        assert(e>i);
        score_t p;//=as_one();
        o << '(';
        const_iterator j=i;
        if (*w==(lm_id_type)sub().unk_id && opt.check_unks) {
            if (opt.openclass_lm) { // [u]
                p=find_prob((j=w),e)*opt.unknown_word_prob;
                o << p;
                goto bo;
            } else { // [c]
                p = opt.unknown_word_prob;
                o << p << ')';
                return p;
            }
        } // else [o]

        for (;j<e;++j) {
            p=find_prob(j,e);
            if (!p.is_zero()) {
                o << p;
                break;
            }
        }
    bo:
        bool found_bo=false;
        for (;i<j;++i) {
            score_t bo=find_bow(i,w);
            if (!bo.is_zero()) {
                found_bo=true;
                o << '+'<<bo;
                p*=bo;
            } else if (found_bo) {
                SBMT_WARNING_STREAM(lm_domain,"Don't try --ngram-shorten for LM "<<name<<" - shorter backoff missing after longer found, which should never happen in a valid LM.");
                o << "+?";
            }
        }
        o << ')';
        return p;
    }


    score_t print_factors_raw(std::ostream &o,const_iterator history_start,const_iterator score_start,const_iterator end,char space=',') const
    {
        score_t p=as_one();
        graehl::word_spacer sp(space);
        for (const_iterator i=score_start;i!=end;++i) {
            o << sp;
            print(o,*i); //FIXME: quote-safety
            o << '=';
            p*=print_factor(o,history_start,i);
        }
        return p;
    }

    /*
    // surrounds with <s> </s>
    template <class indexed_tokens_it>
    score_t print_sentence_details(std::ostream &o,indexed_tokens_it s,indexed_tokens_it end,bool factors=false) const
    {
        lm_ids lm(*this,s,end,true);
        return sub().print_sequence_details_raw(o,lm.begin(),lm.begin()+1,lm.end(),factors);
    }

    template <class indexed_tokens_it>
    score_t print_sequence_details(std::ostream &o,indexed_tokens_it s,indexed_tokens_it score_start,indexed_tokens_it end, bool factors=false) const
    {
        lm_ids lm(*this,s,end,false);
        lm_id_type *b=lm.begin();
        return sub().print_sequence_details_raw(o,b,b+(score_start-s),b+(end-s),factors);
    }
    */

    ngram_options opt;
    C const& sub() const { return *static_cast<C const*>(this); }
    C & sub() { return *static_cast<C*>(this); }

#ifdef STAT_LM
    std::size_t n_calls;
#endif

    void clear()
    {
        own_weight=0;
#ifdef STAT_LM
        n_calls=0;
#endif
    }

    void count_query() const
    {
#ifdef STAT_LM
        ++const_cast<self_type *>(this)->n_calls;
#endif
    }

    fat_ngram_lm(std::string const&type,ngram_options const& opt=ngram_options())
      : type(type)
      , name("lm")
      , opt(opt)
      , loc(gen("en_US.utf-8"))
    {
        opt.check();
        clear();
        set_max_order(0);
    }

 public:


    template <class O>
    void print_stats(O &o) const
    {
#ifdef STAT_LM
        o << " (" << n_calls << " total LM lookups)";
#endif
        sub().stats(o);
    }

  lm_id_type id_existing(std::string const& tok) const {
    return const_cast<self_type *>(this)->id(tok,false);
  }

    lm_id_type id(std::string tok,bool add_unk=false)
    {
        replace_digits(tok);
        return sub().id_raw(tok,add_unk);
    }

    void replace_digits(std::string & s) const
    {
	s = boost::locale::to_lower(s,loc);
        if (opt.lm_at_numclass) replace_digits_with(s,'@');	
    }


    score_t
    prob_unmixed(const_iterator ctx,unsigned len) const
    {
        assert(len>0);
        if (opt.check_unks) {
            lm_id_type const* e = ctx + len;
            lm_id_type const* i = e;
            for(;;) {
                if (i[-1]==(lm_id_type)sub().unk_id)
                    break;
                if (--i==ctx) goto normal;
            }
            if (opt.openclass_lm) {
                if (e==i) {
                    return open_prob(ctx,len); // * opt.unknown_word_prob;
                } else {
                    goto normal;
                }
            } else {
                len=e-i;
                if (len==0) return as_one(); //return opt.unknown_word_prob;
                return open_prob(i,len);
            }
        }
    normal:
        return open_prob(ctx,len);
    }

    score_t
    prob(const_iterator ctx,unsigned len) const
    {
        assert(len>0);
        if (opt.check_unks) {
            lm_id_type const* e = ctx + len;
            lm_id_type const* i = e;
            for(;;) {
                if (i[-1]==(lm_id_type)sub().unk_id)
                    break;
                if (--i==ctx) goto normal;
            }
            if (opt.openclass_lm) {
                if (e==i) {
                    return (open_prob(ctx,len) ^ own_weight) * opt.unknown_word_prob;
                } else {
                    goto normal;
                }
            } else {
                len=e-i;
                if (len==0) return opt.unknown_word_prob;
                return open_prob(i,len) ^ own_weight;
            }
        }
    normal:
        return open_prob(ctx,len) ^ own_weight;
    }

    template <class O,class E,class V>
    inline void log(O &o,char const* probname,const_iterator ctx,E end,V val) const
    {
        o<<probname<<'_'<<name<<'(';
        print(o,ctx,end);
        o<<")="<<val;
    }

    template <class O,class V>
    inline void log_vec(O &o,char const* probname
                        ,const_iterator ctx,const_iterator mid,const_iterator end
                        ,V val) const
    {
        o<<probname<<'_'<<name<<'(';
        print(o,ctx,mid);
        o<<'!';
        print (o,mid,end);
        o<<")="<<val;
    }

    template <class E,class V>
    inline void log(char const* probname,const_iterator ctx,E end,V val) const
    {
#if 0
                           continue_log(str)<<probname<<'_'<<name<<'(';
                           print(continue_log(str),ctx,end);
                           continue_log(str)<<")="<<val;
#endif
        SBMT_VERBOSE_EXPR(ngram_domain,
                        {
                            log(continue_log(str),probname,ctx,end,val);
                        });
    }

    template <class V>
    inline void log_vec(char const* probname,
                        const_iterator ctx,const_iterator mid,const_iterator end
                        ,V val) const
    {
        SBMT_DEBUG_EXPR(ngram_domain,
                          {
                              log_vec(continue_log(str),probname,ctx,mid,end,val);
                          });
    }

    void log_weight(std::string const& wt_name,float wt) const
    {
        SBMT_INFO_STREAM(lm_domain,"For LM "<<name<<" found weight "<<wt_name<<"="<<wt);
    }

    void log_describe() const
    {
        SBMT_INFO_EXPR(lm_domain, {
                continue_log(str)<<"Loaded LM: ";
                describe(continue_log(str));
                continue_log(str)<<" weight "<<name<<'='<<own_weight;
            });
    }

    /// must provide open_prob_len_raw:
    score_t open_prob(const_iterator ctx,unsigned len) const
    {
        count_query();
        score_t ret=sub().open_prob_len_raw(ctx,len);
        log("p",ctx,len,ret);
        return ret;
    }

    score_t open_prob_raw(const_iterator ctx_and_w,const_iterator end) const
    {
        return sub().open_prob_len_raw(ctx_and_w,end-ctx_and_w);
    }

    /*
    score_t open_prob_len_raw(const_iterator ctx_and_w,unsigned len) const
    {
        return sub().open_prob_raw(ctx_and_w,ctx_and_w+len);
    }
    */

  score_t prob(lm_id_type w) const {
    return sub().prob(&w,1);
  }

    score_t prob(const_iterator ctx_and_w,const_iterator end) const
    {
        return sub().prob(ctx_and_w,end-ctx_and_w);
    }

    score_t prob_unmixed(const_iterator ctx_and_w, const_iterator end) const
    {
        return sub().prob_unmixed(ctx_and_w,end-ctx_and_w);
    }

    // this sucks: due to C++ name hiding, the two methods below would need different names to work.

    /// may override (along with open_prob_raw):
    score_t open_prob(const_iterator ctx_and_w,const_iterator end) const {
        return sub().open_prob(ctx_and_w,end-ctx_and_w);
    }


#ifdef DEBUG_LM
#define DEBUG_LM_P(seq,b,end,p) \
        DEBUG_LM_OUT<<"p("; \
        print(DEBUG_LM_OUT,seq,b,end);          \
        DEBUG_LM_OUT << ")="<<p<<"\n"
#else
# define DEBUG_LM_P(seq,b,end,p)
#endif

    inline void bow_interval(component_scores_vec& a, const_iterator i, const_iterator e, const_iterator backoff_end) const
    {
//        assert(a.N>COMP_PROB);
        # ifndef NDEBUG
        feature_t* v=
        # endif
        sub().bow_interval_array(a.v,i,e,backoff_end);
        assert(v==a.v+a.N);
    }


    // sequence_prob family of methods allows empty (or negative length) sequences, which are not scored at all (returning prob=1

    void prob(component_scores_vec &a,const_iterator seq,const_iterator i) const {
      return sequence_prob(a,seq,i,i+1);
    }

    void sequence_prob(component_scores_vec &a,const_iterator seq,const_iterator i,const_iterator end) const
    {
//        assert(a.N>COMP_PROB && a.N>COMP_UNK);
# ifndef NDEBUG
        feature_t* v=
# endif
        sub().sequence_prob_array(a.v,seq,i,end);
# ifndef NDEBUG
        assert(v==a.v+a.N);
#         endif
    }

    // override this for multi-lm only, probably
    feature_t* bow_interval_array(feature_t* v, const_iterator i, const_iterator e, const_iterator backoff_end) const
    {
        *v = std::make_pair(nameid,v->second * bow_interval(i,e,backoff_end));
        return opt.is_open() ? v + 1 : v + 2;
    }

    //override for multi-lm only
    feature_t* sequence_prob_array(feature_t* v,const_iterator history_start,const_iterator score_start,const_iterator end) const
    {
        //FIXME: can be implemented more efficiently by a prob_array that counts unk just once
        //FIXME: copy/paste shared with print_sequence_details_raw
        score_t p_just_lm=this->sequence_prob_unmixed(history_start,score_start,end);
        unsigned n_unk=this->count_unk(score_start,end);
        score_t feat_unk(n_unk,as_neglog10());
        //if (!opt.is_open()) {
        //    score_t p_unk(opt.unknown_word_prob,n_unk);
        //    p_just_lm/=p_unk;
        //}

        *v = std::make_pair(nameid, v->second * p_just_lm);
        ++v;
        if (not opt.is_open()) {
            *v = std::make_pair(nameunkid,v->second * feat_unk);
            ++v;
        }
        return v;
    }

    // scores words i=start..end given seq..i
    score_t sequence_prob(const_iterator seq,const_iterator i,const_iterator end) const
    {
        score_t p_unk = opt.is_open()
                      ? as_one()
                      : (opt.unknown_word_prob ^ count_unk(i,end));

        return (sequence_prob_unmixed(seq,i,end) ^ own_weight) * p_unk;
    }

    score_t sequence_prob_unmixed(const_iterator seq, const_iterator i, const_iterator end) const
    {
        const_iterator b=i;
        score_t p=as_one();
        if (i<end) {
            do p*=sub().prob_unmixed(seq,++i); while (i<end);
          DEBUG_LM_P(seq,b,end,p);
          log_vec("sequence_prob_unmixed",seq,b,end,p);
        }
        return p;
    }

    bool is_null(lm_id_type id) const
    {
        return id==sub().null_id;
    }

    score_t feat_unk(const_iterator i,const_iterator end) const
    {
        return score_t(count_unk(i,end),as_neglog10());
    }

    unsigned count_unk(const_iterator i,const_iterator end) const
    {
        unsigned c=0;
        for (;i<end;++i)
            if (*i==(lm_id_type)sub().unk_id) ++c;
        return c;
    }

    // scores words i=start..end given seq..i
    score_t sequence_prob_stop_null(const_iterator seq,const_iterator i,const_iterator end) const
    {
        const_iterator b=i;
        score_t p=as_one();
        while (i<end && !is_null(*i)) //!sub().is_null(*i)
            p*=prob(seq,++i);
        DEBUG_LM_P(seq,b,i,p);
        log_vec("sequence_prob_stop_null",seq,b,i,p);
        return p;
    }

    // scores words i..end (1gram, 2gram ...)
    score_t sequence_prob(const_iterator i,const_iterator end) const
    {
        return sequence_prob(i,i,end);
    }


    // scores words i..end (1gram, 2gram ...)
    score_t sequence_prob_stop_null(const_iterator i,const_iterator end) const
    {
        return sequence_prob_stop_null(i,i,end);
    }

    // i and j index into array starting at seq.  prob of words i...(j-1) given 0...i-1
    score_t sequence_prob(const_iterator seq,unsigned i,unsigned j) const
    {
#if 1
        return sequence_prob(seq,seq+i,seq+j);
#else
        score_t p=as_one();
        while (i<j)
            p*=sub().prob(seq,++i);
        return p;
#endif
    }

    score_t sequence_prob_stop_null(const_iterator seq,unsigned i,unsigned j) const
    {
#if 1
        return sequence_prob_stop_null(seq,seq+i,seq+j);
#else
        score_t p=as_one();
        while(i<j && !is_null(seq[i]))
            p*=sub().prob(seq,++i);
        return p;
#endif
    }

    typedef std::vector<lm_id_type> word_vec;

    // scores words i..end (1gram, 2gram ...)
    score_t sequence_prob(word_vec const& v,unsigned i,unsigned j) const
    {
        return sequence_prob(&v[0],i,j);
    }

    score_t sequence_prob_stop_null(word_vec const& v,unsigned i,unsigned j) const
    {
        return sequence_prob_stop_null(&v[0],i,j);
    }

    /// multiplies into score backoffs [i..backoff_end) * ... * [(e-1)..backoff_end) (or nothing if e<=i)
    inline score_t bow_interval(const_iterator i,const_iterator e,const_iterator backoff_end) const
    {
        score_t ret=as_one();
#ifdef DEBUG_LM
        DEBUG_LM_OUT<<"p_bo_interval(";
        print(DEBUG_LM_OUT,i,e,backoff_end);
#endif
        shorten_history(i,backoff_end); //        graehl::maybe_increase_max(i,1+backoff_end-max_order_cached);

#ifdef DEBUG_LM
        const_iterator e_orig=e;
#endif
        while (e>i) {
            --e;
            score_t bo=sub().find_bow(e,backoff_end);
#ifdef DEBUG_LM
            print(DEBUG_LM_OUT<<" p_bo(",e,backoff_end)<<")="<<bo;
#endif
            if (bo.is_zero())
#ifdef BASE_NGRAM_BOW_WITH_HOLES
                continue;
#else
                break;
#endif
            ret *= bo;
        }
#ifdef DEBUG_LM
        DEBUG_LM_OUT << ")="<<score/score_orig<<"\n";
#endif
        log_vec("bow_interval",i,e,backoff_end,ret);
        return ret;
    }

    /// returns r such that [i,r) is the longest known history (i.e. length maxorder-1 max)
    const_iterator longest_prefix(const_iterator i,const_iterator end) const
    {
        const_iterator ret=sub().longest_prefix_raw(i,end);
        log("prefix_shorten_by",i,end,end-ret);
        return ret;
    }

    /// reverse of longest_prefix (returns r such that [r,end) is the longest seen history)
    const_iterator longest_suffix(const_iterator i,const_iterator end) const
    {
        const_iterator ret=sub().longest_suffix_raw(i,end);
        log("suffix_shorten_by",i,end,ret-i);
        return ret;
    }

    // returns score_t(as_zero()) if the string doesn't exist, prob if end-i = maxorder, bo otherwise
    score_t find_bow(const_iterator i,const_iterator end) const
    {
        count_query();
        score_t ret=sub().find_bow_raw(i,end);
        log("bow",i,end,ret);
        return ret;
    }

    score_t find_prob(const_iterator i,const_iterator end) const
    {
        count_query();
        score_t ret=sub().find_prob_raw(i,end);
        log("prob",i,end,ret);
        return ret;
    }

    /// ensure we don't query for any backoff longer than max-1 len
    void shorten_history(const_iterator &i,const_iterator end) const
    {
#ifdef DEBUG_LM_SHORTEN_HISTORY
        int len=end-i;
        if (len > max_order_cached-1)
            log("shorten_history",i,end,len);
#endif
        graehl::maybe_increase_max(i,1+end-max_order_cached);
    }

    void assert_short_history(const_iterator i,const_iterator end) const
    {
        assert(end-i<max_order_cached);
    }

    /// ensure we don't query for any prob w/ history longer than max-1 words
    void shorten_ngram(const_iterator &i,const_iterator end) const
    {
        graehl::maybe_increase_max(i,end-max_order_cached);
    }

    /// ensure we don't query for any prob w/ history longer than max-1 words
    void shorten_ngram_len(const_iterator &i,unsigned &len) const
    {
        if ((int)len > max_order_cached) {
            i+=(len-max_order_cached);
            len=max_order_cached;
        }
    }

    /// may override: returns r such that [b,r) is the longest known history (i.e. length maxorder-1 max).  PRE: end-i is maxorder-1 or less
    const_iterator longest_prefix_raw(const_iterator i,const_iterator end) const
    {
        assert_short_history(i,end);
        while (i<end) { // longest to shortest
            if (!sub().find_bow(i,end).is_zero()) // not 0 -> have seen
                return end;
            --end;
        }
        return end; //==i
    }

    /// may override: reverse of longest_prefix (returns r such that [r,end) is the longest seen history).  PRE: end-i is maxorder-1 or less
    const_iterator longest_suffix_raw(const_iterator i,const_iterator end) const
    {
        assert_short_history(i,end);
        while (i<end) { // longest to shortest
            if (!sub().find_bow(i,end).is_zero())  // not 0 -> have seen
                return i;
            ++i;
        }
        return i; //==end
    }

    std::string const& word(lm_id_type id) const
    {
        static const std::string bo_word="#";
        static const std::string null_word="*";
        if (id==(lm_id_type)sub().bo_id)
            return bo_word;
        else if (id==(lm_id_type)sub().null_id)
            return null_word;
        return sub().word_raw(id);
    }

    //FIXME: quote-safety
    template <class C, class T>
    std::basic_ostream<C,T>&
    print(std::basic_ostream<C,T>& o, lm_id_type w) const
    {
        if (w == (lm_id_type)sub().bo_id) return o << "#";
        else if (w == (lm_id_type)sub().null_id) return o << "*";
        return o << '"' << gusc::escape_c(sub().word_raw(w)) << '"';

    }

    template <class C, class T>
    std::basic_ostream<C,T>&
    print(std::basic_ostream<C,T>& o, std::vector<lm_id_type> const& v) const
    {
        return print(o,&v[0],&v[0]+v.size());
    }

    template <class C, class T>
    std::basic_ostream<C,T>&
    print(std::basic_ostream<C,T>& o, const_iterator ctx, unsigned len) const
    {
        return print(o,ctx,ctx+len);
    }

    template <class C, class T>
    std::basic_ostream<C,T>&
    print(std::basic_ostream<C,T>& o, const_iterator ctx, const_iterator e, char space=',') const
    {
        graehl::word_spacer sp(space);
        for(const_iterator i=ctx;i<e;++i) {
            o << sp;
            print(o,*i);
        }
        return o;
    }

    template <class C, class T>
    std::basic_ostream<C,T>&
    print(std::basic_ostream<C,T>& o, const_iterator ctx, unsigned i, unsigned e) const
    {
        return print(o,ctx,ctx+i,ctx+e);
    }

    template <class C, class T>
    std::basic_ostream<C,T>&
    print(std::basic_ostream<C,T>& o, const_iterator ctx, const_iterator i, const_iterator e) const
    {
//        if (i>=e) return o;
        if (i>ctx && i<=e)
            print(o,ctx,i)<<"!!";
        return print(o,i,e);
    }

public:
    void finish()
    {
        cache_max_order();
    }

};


}



#endif
