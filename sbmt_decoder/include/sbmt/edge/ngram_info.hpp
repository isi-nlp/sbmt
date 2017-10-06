#ifndef SBMT_EDGE_ngram_info_hpp_
#define SBMT_EDGE_ngram_info_hpp_
//\TODO : conversion of the vocab index ids.
//\TODO : put implementation into ipp file.
//\TODO : add test cases.
/* TODO? do any of these special tokens need to be skipped (e.g. epsilon).  i'd guess not, because they wouldn't appear in lmstring?

[sbmt.clm][info]: TM word <unknown-word/> unknown to clm-left: 1=<unk>
[sbmt.clm][verbose]: e-context map: <unknown-word/> ngram-id=3 ngram-word=<unknown-word/> left-id=1 left-word=<unk>
[sbmt.clm][info]: TM word <epsilon/> unknown to clm-left: 1=<unk>
[sbmt.clm][verbose]: e-context map: <epsilon/> ngram-id=4 ngram-word=<epsilon/> left-id=1 left-word=<unk>
[sbmt.clm][info]: TM word <separator/> unknown to clm-left: 1=<unk>
[sbmt.clm][verbose]: e-context map: <separator/> ngram-id=5 ngram-word=<separator/> left-id=1 left-word=<unk>
[sbmt.clm][info]: TM word <top> unknown to clm-left: 1=<unk>
[sbmt.clm][info]: TM word <unknown-word/> unknown to clm-left: 1=<unk>
[sbmt.clm][verbose]: e-context map: <unknown-word/> ngram-id=3 ngram-word=<unknown-word/> left-id=1 left-word=<unk>
[sbmt.clm][info]: TM word <epsilon/> unknown to clm-left: 1=<unk>
[sbmt.clm][verbose]: e-context map: <epsilon/> ngram-id=4 ngram-word=<epsilon/> left-id=1 left-word=<unk>
[sbmt.clm][info]: TM word <separator/> unknown to clm-left: 1=<unk>
[sbmt.clm][verbose]: e-context map: <separator/> ngram-id=5 ngram-word=<separator/> left-id=1 left-word=<unk>
[sbmt.clm][info]: TM word <top> unknown to clm-left: 1=<unk>
*/
//#define DEBUG_LM
//#define DEBUG_LM_OUT std::cerr

#define NGRAM_EXTRA_STATS
#define LM_EQUIVALENT

/**
      LM_EQUIVALENT shortens equivalent left and right words

   Using a real (bitext) 5gram LM and some real (short) sentences:

    Reduced border words on average from 3.25 (on each side) to 2.58, out of a
    possible 4.  While only a 20% reduction, this means we should expect more
    recombination and more mileage out of our beams (per time and space).  This
    has to be balanced against the cost of computing the context reductions in
    the first place, however.

    LM scores are still correct in all cases (verified against sending whole
    sentences to LW LangModel program with lw-lm-score.pl) avg(N=7457):

    lw/decoder lmcost
    ratio=[0.99999302586705896/0.99999991990884118a/1.0000027208071001]


   SUMMARY OF NUMBERS - "avg(N=<n-occurences>):
   text-with-[min/avg/max]-replacing-numbers": avg(N=109): LM item border words:
   ([3.21222/3.256181927/3.43701] average) reduced (by LM equivalence) to
   ([2.52913/2.583001743/2.71198] average) over
   [1514108/1842160886.8991/9410868022] contexts

note that many of the items were less than 5 words since these were mostly short
sentences.  also note that for 5gram, the maximum # of border words would be
4. (N-1)

Evidence of better search (beams were too large to show much): all 1best
totalcost were same, except 3 out of 109, which improved with LM_EQUIVALENT.

Caveat: because I didn't have any weights tuned w/ 5gram integrated LM, many of
the translations were very short and/or used GLUE (but there were several long
translations with full parses).

Out of the first 50 sentences, evidence of increased recombination (counting only the *useful* parts of the chart):

(NEW recomb.) Parse forest has (loops excluded) [22/167.52/2935] trees in
[151/767.24/1371] items connected by [172/834.46/1448] edges.

(OLD) Parse forest has (loops excluded) [22/132.08/1355] trees in [156/770.
76/1423] items connected by [178/839/1848] edges.

     Same beams, slightly less edges/items, significantly (20%) more trees represented.

                           Looking at *all* the edges generated (including ones that aren't part of a full parse):

LM_EQUIVALENT: [776187/43782354.9174/980825023] edges created
old (regular) combination: [716284/39567094.1927/816948988] edges created

Overall runtime was 17% slower; 10% of that due to more edges created, the rest to extra work done per ngram_info combination.

     So far I feel like this is a slight improvement and nothing to write home about.

     Test with higher order ngrams, tighter beams, and longer sentences might be
     more demonstrative.


 */

                                //#include <sbmt/io/formatted_logging_msg>
#include <boost/shared_ptr.hpp>
#include <sbmt/ngram/LWNgramLM.hpp>
#include <sbmt/ngram/dynamic_ngram_lm.hpp>
#include <sbmt/token/indexed_token.hpp>
#include <sbmt/edge/component_scores.hpp>
#include <sbmt/feature/feature_vector.hpp>
#include <gusc/generator/single_value_generator.hpp>
#include <graehl/shared/dynamic_array.hpp>
# include <sbmt/edge/info_base.hpp>

namespace sbmt {
    
template <class Grammar>
struct ngram_rule_data {
    typedef indexed_lm_string type;
    typedef type const& return_type;
    static return_type value( Grammar const& grammar
                            , typename Grammar::rule_type r
                            , size_t lmstrid )
    {
        return grammar.template rule_property<indexed_lm_string>(r,lmstrid);
    }
};

template <class Grammar>
struct ngram_rule_data<Grammar const> : ngram_rule_data<Grammar> {};

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(clm_domain,"clm",root_domain);
SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(clm_component_domain,"component",clm_domain);

//SBMT_SET_DOMAIN_LOGGING_LEVEL(clm_domain,info);

////////////////////////////////////////////////////////////////////////////////
///
/// templated ngram: advantage: lower memory usage, and stack allocated
///                  disadvantage: will have to declare the order of ngrams in
///                                code.
///
/// N is the order of the ngram.
////////////////////////////////////////////////////////////////////////////////

//the N=1 specialization requires many dummy implementations.
//refactor methods into free functions applying to both where possible
//(should only have to define basic end_full,begin_full accessors).  mostly
//already done in lm_phrase.ipp

template <unsigned N,class LMIDType = unsigned>
class ngram_info : public info_base< ngram_info<N,LMIDType> >
{
public:
    typedef LMIDType lm_id_type;
    typedef lm_id_type *iterator;
    typedef lm_id_type const* const_iterator;
    typedef ngram_info<N,LMIDType> info_type;
//    BOOST_STATIC_CONSTANT(unsigned,ctx_len=N-1);
    enum {ctx_len=N-1};
private:
    //! left/right boundary words
    //! Undefined words are represented via .
    //! Since N is the order of ngram, we need only N-1 context words in
    //! each direction (left or right).
    lm_id_type lr[2][ctx_len]; // 2 rows, ctx_len columns
    //ary[i][j] is really aryp[i * ctx_len + j]
 public:
     ngram_info()
     {
         for(unsigned j = 0; j < NUM_BOUNDARIES; ++j)
              for(unsigned i  = 0; i < ctx_len; ++i)
                lr[j][i]=lm_id_type();
     }
     ngram_info& operator=(info_type const& o)
     {
         for(unsigned j = 0; j < NUM_BOUNDARIES; ++j)
             for(unsigned i  = 0; i < ctx_len; ++i)
                 lr[j][i]=o.lr[j][i];
         return *this;
     }

    template <class C, class T, class LMType>
    std::basic_ostream<C,T>&
    print(std::basic_ostream<C,T>& o, LMType const& lm) const ;

    template <class C, class T>
    std::basic_ostream<C,T>& print_self(std::basic_ostream<C,T>& o) const;

    template <class C, class T, class TF>
    std::basic_ostream<C,T>& print_self(std::basic_ostream<C,T>& o, TF& tf) const {
        return print_self(o);
    }

    enum {NUM_BOUNDARIES = 2, HASH_OFFSET=100};

    //! compare the context words
    bool equal_to(info_type const& other) const
    {
        return equal_to(other,ctx_len);
    }

    bool equal_to(info_type const& other, unsigned short const& context_len) const;

    //! hash the context words.
    std::size_t hash_value() const;
    std::size_t hash_value(unsigned context_len) const;
//    INFO_HASH_EQUAL_TO

    //! Returns the boundary word.
    //! \param j 0 means left, 1 means right
    //! \param i the token index.
    lm_id_type const& operator()(unsigned j, unsigned i) const
    { return lr[j][i]; }

    lm_id_type& operator()(unsigned j, unsigned i)
    { return lr[j][i]; }
};

////////////////////////////////////////////////////////////////////////////////

template <class LMIDType>
class ngram_info<1,LMIDType> : public info_base<ngram_info<1,LMIDType> >
{
    typedef ngram_info<1,LMIDType> info_type;
 public:
    typedef LMIDType lm_id_type;
    typedef lm_id_type *iterator;
    typedef lm_id_type const* const_iterator;
    enum {ctx_len=0};

    template <class C, class T, class LM>
    std::basic_ostream<C,T>&
    print(std::basic_ostream<C,T>& o, LM const& lm) const
    {
        return print_self(o);
    }
    template <class C, class T>
    std::basic_ostream<C,T>& print_self(std::basic_ostream<C,T>& o) const
    {
        return o;
    }
    template <class C, class T, class TF>
    std::basic_ostream<C,T>&
    print_self(std::basic_ostream<C,T>& o, TF& tf) const
    {
        return print_self(o);
    }

    /*
    static bool has_component_features()
    {
        return true;
    }
    static std::string component_features() {
        return "lm";
    }
    */
    enum {NUM_BOUNDARIES = 2, HASH_OFFSET=100};



    //! compare the context words
    bool equal_to(info_type const& other, unsigned context=1) const
    { return true; }

    //! hash the context words.
    std::size_t hash_value(unsigned context=1) const
    { return 0; }

    lm_id_type const& operator()(unsigned j, unsigned i) const // don't use or bad things happen!
    { return *(lm_id_type const*)this; }

    lm_id_type & operator()(unsigned j, unsigned i)  // don't use or bad things happen!
    { return *(lm_id_type*)this; }

};

////////////////////////////////////////////////////////////////////////////////

/*
// warning: not multi-LM safe.
template<class O,unsigned N,class LM>
O& operator <<(O& o, ngram_info<N,LM> const& info)
{
    info.print_self(o);
    return o;
}
*/

////////////////////////////////////////////////////////////////////////////////

template<class C, class T, class TF, unsigned N,class LM>
void print(std::basic_ostream<C,T> &o, ngram_info<N,LM> const& info, TF const&tf)
{
    info.print_self(o,tf);
}

template<class C, class T, class TF, unsigned N,class LM>
void print(std::basic_ostream<C,T> &o, ngram_info<N,LM> const& info, LM const& lm)
{
    info.print(o,lm);
}

////////////////////////////////////////////////////////////////////////////////

//TODO: static char const* ?
#define CLM_LEFT "clm-left"
#define CLM_RIGHT "clm-right"
// remember to sync below with extract (ghkm) and graehl/clm/extract.clm.sh
#define CLM_F_SOS "<foreign-sentence>"
// note: since CLM_F_SOS is actually part of foreign lattice for glue rules, this means our view of lattice will have two repetitions of it; but the first should have p=1 (or not be scored if we ignore the TOP token in clm-scoreable)
#define CLM_F_EOS "</foreign-sentence>"
template <unsigned N, class LM>
class ngram_info_factory {
public:
    enum {ctx_len=N-1};
    template <class O>
    void print_stats(O &o) const;

    void reset() {}

    typedef LM lm_type;
    typedef typename lm_type::lm_id_type lm_id_type;
    typedef ngram_info<N,lm_id_type> info_type;
    typedef boost::tuple<info_type,score_t,score_t> result_type;
    typedef gusc::single_value_generator<result_type> result_generator;
    typedef ngram_info_factory<N,LM> self_type;
    bool shorten_left,shorten_right;
    load_lm lm;
    indexed_token_factory& dict;

    typedef graehl::dynamic_array<lm_id_type> adj_t;
    typedef graehl::fixed_array<lm_id_type> best_adj_t;
    typedef graehl::fixed_array<adj_t> adjs_t; // unlike std::vector, efficient to contain array inside array (probably some boost array type would suffice nowadays)
    typedef graehl::dynamic_array<lm_id_type> idmap_t;


    //TODO: drop idmap_t, by having every ngram_info just store indexed_token ids and indirect when asking lm, clm[0], clm[1] seperately - also allows different digit-normalization etc.
  //!potentially add words from this lm's vocab to vocab of other, result is table from other id->our id.  (multi-lm will definitely add words that aren't unknown to every lm).  //done: supply all lms (todo: biglm!) with option to add to vocab to plain lm
    // alternative: (share common vocab ids natively across all lms?); difficulty w/ biglm (need to wrap my own) anticipated; lwlm is ok.  alternative: indexed_token ids as interface to lm. add_unk should make the map info-preserving (we distinguish all of our known words)
    static void inverse_map(idmap_t &map,indexed_token_factory &tf,LM &me,LM &other,char const* name,bool add_unk=false) //TODO: test add_unk=true
    {
        BOOST_FOREACH(indexed_token e, tf.native_words()) {
            std::string const& l=tf.label(e);
            unsigned myid=id_warn(me,l,false,name);
            unsigned oid=id_warn(other,l,add_unk&&myid!=me.unk_id,"dynamic-lm-ngram");
            //note: this may not really be a 1-1 map.
            SBMT_DEBUG_MSG(clm_domain,"e-context map: %6% ngram-id=%2% ngram-word=%3% %1%-id=%4% %1%-word=%5%",name%oid%other.word_raw(oid)%myid%me.word_raw(myid)%l);
            graehl::at_expand(map,oid)=myid;
        }
    }
    static unsigned id_warn(LM &me,std::string const& l,bool add_unk,char const* name)
    {

        unsigned myid=me.id(l,add_unk);
        if (myid==me.unk_id)
            SBMT_VERBOSE_MSG(clm_domain,"TM word %1% unknown to clm-%2%: %3%=%4%",l%name%myid%me.word_raw(myid));
        return myid;
    }

    struct clm_side
    {
        inline score_t prob(lm_id_type *p,lm_id_type *e,char const* name,bool component=false) const
        {
            score_t prob=lm.lm().open_prob_len_raw(p,e-p); //TODO: do we want raw, or modified (closed lm etc.)?  no, no clm-left-unk feature, please.  open always.  and digit mapping already happened in id->id space.
            if (prob.is_zero()) {
                SBMT_VERBOSE_EXPR(clm_domain,{lm.lm().log(continue_log(str)<<"(probably new f word which was an e word in training) replacing 0 prob w/ unk prob="<<unk_prob<<" ",name,p,e,prob);});
                prob=unk_prob;
            }
            //refactor below: ?
            SBMT_DEBUG_EXPR(component?clm_component_domain:clm_domain,{lm.lm().log(continue_log(str),name,p,e,prob);});
            return prob;
        }
        load_lm lm;
        idmap_t map; //FIXME: make sure that info_factory isn't copied again after being wrapped in any_info_factory, so that map isn't built then discarded (would have to make this a shared_ptr if so).  also, compute this in constructor rather than for every sentence's factory, because it doesn't change unless grammar does. 3/24/2011. yes, shared_ptr<holder<info_factory> > - holder has the copy.
        score_t score_left(info_type const& info,span_index_t sp,bool component=false) const
        {
            lm_id_type event[N];
            lm_id_type *e=&event[N];
            lm_id_type *p=e;
            *--p=best_adj[sp];
            for (lm_id_type const *i=&info(0,0),*e=&info(0,ctx_len);i<e;++i) {
                lm_id_type ew=*i;
                //FIXME: logic copied from lm_phrase.ipp ngram_info_accessors but I didn't want to reorg so it appears first (or this later).
                if (ew==(lm_id_type)lm_type::null_id
#ifdef LM_EQUIVALENT
                    || ew==(lm_id_type)lm_type::bo_id
#endif
                    ) break;
                *--p=map[ew];
            }
            return prob(p,e,"clm-left",component);
        }
        score_t score_right(info_type const& info,span_index_t sp,bool component=false) const
        {
            lm_id_type event[N];
            lm_id_type *e=&event[N];
            lm_id_type *p=e;
            *--p=best_adj[sp];
            for (lm_id_type const*e=&info(1,0),*i=&info(1,ctx_len-1);i>=e;--i) {
                lm_id_type ew=*i;
                //FIXME: logic copied from lm_phrase.ipp ngram_info_accessors but I didn't want to reorg so it appears first (or this later).
                if (ew==(lm_id_type)lm_type::null_id) break;
                *--p=map[ew];
            }
            return prob(p,e,"clm-right",component);
        }

        score_t score(bool r,info_type const& info,span_index_t p,bool component=false) const
        {
            return r ? score_right(info,p,component) : score_left(info,p,component);
        }

        score_t wt_score(bool r,info_type const& info,span_index_t p) const
        {
            if (is_null()) return as_one();
            return score(r,info,p).pow(wt);
        }

        unsigned feat_id;

        template <class O>
        O component_score(bool r,info_type const& info,span_index_t p,O o) const
        {
            if (!is_null()) {
                *o=std::make_pair(feat_id,score(r,info,p,true));
                ++o;
            }
            return o;
        }
        lm_id_type unk_id;
        score_t unk_prob;
        void set_lm(ngram_ptr plm)
        {
            lm=load_lm(plm);
            if (!lm.is_null()) {
                unk_id=lm.lm().unk_id;
                unk_prob=lm.lm().prob(&unk_id,1);
            }
        }
        void set_wt(weight_vector const& wv, feature_dictionary &fn, char const* name)
        {
            wt=wv[feat_id=fn.get_index(name)];
        }

        bool is_null() const { return lm.is_null(); }
        adjs_t adjs;
        best_adj_t best_adj;
        void init_adjs(unsigned s)
        {
            if (is_null()) return;
            adjs.reinit(s);
        }

        std::string const& word(lm_id_type id) const
        {
            return lm.lm().word_raw(id);
        }

        void record_adj(indexed_token_factory& dict,indexed_token t,span_index_t p,char const* dir)
        {
            if (is_null()) return;
            std::string const& name=dict.label(t);
            lm_id_type f=lm.lm().id(name);
            std::string const& lmname=word(f);
            if (lmname!=name)
                SBMT_TERSE_MSG(clm_domain,"lattice-adj: unknown %4% foreign word F=%1% lm-word=%3% (lm-id=%2%)" ,name%f%lmname%dir);
//            SBMT_VERBOSE_EXPR(clm_domain,{continue_log(str)<<"F:"<<name<<" lm-id="<<f<<" lm-word="<<word(f);});
            SBMT_VERBOSE_MSG(clm_domain,"lattice %4% of node=%5% F=%1% lm-id=%2% lm-word=%3%",name%f%lmname%dir%p);
            adjs[p].push_back(f); // also add any unk, because otherwise we're going to mistakenly add a sos/eos in finish_adj
        }
        /* foreign-outside context is a single word, even for lattices.  we record a default start/end foreign word whenever there are no left/right-incident edges; otherwise we keep use the most probable incident word */
        void finish_adjs(std::string const& none,char const* dir)
        {
            finish_adjs(lm.lm().id(none),dir);
        }

        void finish_adjs(lm_id_type none,char const* dir) // none is <s> or </s> (for L or R)
        {
            if (is_null()) return;
            lm_type &l=lm.lm();
            unsigned i=0,n=adjs.size();
            best_adj.reinit(n);
            for(;i<n;++i) {
                lm_id_type &b=best_adj[i];
                b=none;
                score_t bestp=as_zero();
                BOOST_FOREACH(lm_id_type id,adjs[i]) {
                    score_t p=l.open_prob_len_raw(&id,1);
                    SBMT_PEDANTIC_MSG(clm_domain,"new best lattice %6% of pos=%2% %1% lm-id=%3% lm-word=%4% unigram p=%5%",word(none)%i%id%word(id)%p%dir);
                    if (p>bestp) {
                        b=id;
                        bestp=p;
                    }
                }
                if (adjs[i].empty()) {
                    adjs[i].push_back(none);
                    SBMT_PEDANTIC_MSG(clm_domain,"default lattice %4% of pos=%1% lm-id=%2% lm-word=%3%",i%none%word(none)%dir);
                }
            }
        }


        void finish_map(indexed_token_factory &dict,lm_type &dyn,char const* name,bool add_unk=true)
        {
            if (is_null()) return;
            inverse_map(map,dict,lm.lm(),dyn,name,add_unk);
        }

        double wt;
    };

    bool clm_virtual;
    clm_side clm[2];
    enum { L=0,R=1 };
    bool have_clm;

    ngram_info_factory( weight_vector const& wv
                      , feature_dictionary& fn
                      , lattice_tree const& lattice
                      , indexed_token_factory& dict
                      , boost::shared_ptr< std::vector<indexed_token::size_type> > lmmap
                      , ngram_ptr ngram_lm
                      , unsigned lmstrid
                      , bool shorten = false
                      , ngram_ptr clm_left=ngram_ptr()
                      , ngram_ptr clm_right=ngram_ptr()
                      , bool clm_virtual = false
        )
        :
        lm(ngram_lm)
        , dict(dict)
        , clm_virtual(clm_virtual)
        , lmstrid(lmstrid)
        , lmmap(lmmap)
    {
        clm[L].set_lm(clm_left);
        clm[R].set_lm(clm_right);
        clm[L].set_wt(wv,fn,CLM_LEFT);
        clm[R].set_wt(wv,fn,CLM_RIGHT);

//        clm[L].wt=wv[clm[L].feat_id=fn.get_index(CLM_LEFT))];
//        clm[R].wt=wv[clm[R].feat_id=fn.get_index(CLM_RIGHT))];


        have_clm=!(clm[L].is_null()&&clm[R].is_null());
        /*
        clml=get_context_lm(clmlw,clm_always,lm.get(),wv,fn,CLM_LEFT,CLM_LEFT_LM);
        clmr=get_context_lm(clmrw,clm_always,lm.get(),wv,fn,CLM_RIGHT,CLM_RIGHT_LM);
        */

        shorten_left=shorten&&clm[L].is_null(); //TODO: determine how to support left shortening properly for backwards left-clm (because e words never occur as predicted events in a clm; only as contexts)
        shorten_right=shorten; //WARNING: training for regular lm must include all the contexts that are used in cross-language LM (i.e. use same bilingual data for both)

        clm[0].finish_map(dict,lm.lm(),"left",true);
        clm[1].finish_map(dict,lm.lm(),"right",true);

	//        if (have_clm) {  // you can run w/ just r or l context lms.  guard inside clm[i]
      //  unsigned s=lattice.root().span().right()+1;
        //    clm[L].init_adjs(s);
	//  clm[R].init_adjs(s);
	//  lattice.visit_edges(*this);
	//  clm[L].finish_adjs(CLM_F_SOS,"left");
	//  clm[R].finish_adjs(CLM_F_EOS,"right");
        //}
    }

    lm_id_type id(load_lm const&lm,indexed_token t) const
    {
        lm_id_type mid = lm.lm().id(dict.label(t));
        //if (mid == lm.lm().unk_id) std::cerr << dict.label(t) << " is unknown to lm\n";
        return mid;
    }
    void operator()(lattice_edge const& e)
    {
        indexed_token f=e.source;
        if (f.is_null()) return;
        assert(f.type()==foreign_token);
        clm[L].record_adj(dict,f,e.span.right(),"left"); // we're to left of our right end point
        clm[R].record_adj(dict,f,e.span.left(),"right"); // and we're to right of our left end point
    }

    template <class Grammar>
    std::string hash_string(Grammar const& grammar, info_type const& info) const
    {
        std::stringstream sstr;
        info.print(sstr,lm.lm());
        return sstr.str();
    }

    bool clm_scoreable(token_type_id t) const
    {
        return t!=top_token &&
            (clm_virtual || t==tag_token);
//excludes TOP, but that's ok because prob of f start/end = 1 given e start/end.  actually, our clm doesn't even include english <s> </s> contexts because those are never inside anything
    }

    template <class Grammar, class ConstituentIterator>
    result_generator
    create_info( Grammar& grammar
               , typename Grammar::rule_type r
               , span_t const& span
               , boost::iterator_range<ConstituentIterator> rng )
    {
        lmkids lmk(rng);
        token_type_id t=grammar.rule_lhs(r).type();
        
        //bool is_toplevel = false;
        typename ngram_rule_data<Grammar>::return_type 
            lmstr = ngram_rule_data<Grammar>::value(grammar,r,lmstrid);
        bool is_toplevel = t == top_token and not start_symbol(lmk,lmstr);
        boost::tuple<info_type,score_t,score_t> ret;
        compute_ngrams( boost::get<0>(ret) // frame 9
                      , boost::get<1>(ret)
                      , boost::get<2>(ret)
                      , grammar
                      , is_toplevel
                      , lmstr
                      , lmk );
        if (clm_scoreable(t))
            for (unsigned i=0;i<2;++i)
                boost::get<1>(ret)*=clm[i].wt_score(i,boost::get<0>(ret),span.lr(i));
#if 0
        std::cerr << "\nLM [";
        copy(lmstr.begin(), lmstr.end(), std::ostream_iterator< lm_token<indexed_token> >(std::cerr,","));
        std::cerr << "] + [";
        BOOST_FOREACH(constituent<info_type> c,rng) {
	  if (c.info()) std::cerr << hash_string(grammar,*c.info()) << ' ';
	  else std::cerr << '*' << ' ';
        }
        std::cerr << "] -> " << hash_string(grammar,boost::get<0>(ret)) << ' ' << boost::get<1>(ret) << ' ' << boost::get<2>(ret) << '\n';
#endif
        return ret;
    }
    
    template <class Grammar, class ConstituentIterator, class ScoreOutputIterator>
    boost::tuple<ScoreOutputIterator,ScoreOutputIterator>
    component_scores( Grammar const& grammar
                    , typename Grammar::rule_type r
                    , span_t const& span
                    , boost::iterator_range<ConstituentIterator> constituents
                    , info_type const& result
                    , ScoreOutputIterator out
                    , ScoreOutputIterator hout )
    {
        span_t target;

        lmkids lmk(constituents);
        token_type_id t=grammar.rule_lhs(r).type();
        
        //bool is_toplevel = false;
        typename ngram_rule_data<Grammar>::return_type 
            lmstr = ngram_rule_data<Grammar>::value(grammar,r,lmstrid);
        bool is_toplevel = t == top_token and not start_symbol(lmk,lmstr);
        score_t ignore;
        info_type ignoretoo;
        component_scores_vec vec(lm.lm().n_components());
        component_scores_vec hvec(lm.lm().n_components());
         //FIXME pass output arg refs since no RVO for tuple of vec? no, don't care about performance of component scores.
        boost::tie(vec,hvec) = compute_ngrams( ignoretoo
                                             , grammar
                                             , is_toplevel
                                             , lmstr
                                             , lmk );
        assert(ignoretoo == result);
        out=std::copy(vec.v, vec.v + vec.N, out);
        hout = std::copy(hvec.v, hvec.v + hvec.N, hout);
        if (clm_scoreable(t))
            for (unsigned i=0;i<2;++i)
                out=clm[i].component_score(i,result,span.lr(i),out);
        return boost::make_tuple(out,hout);
    }
    
    bool deterministic() const { return true; }

    template <class Gram>
    score_t rule_heuristic(Gram& gram, typename Gram::rule_type r) const;

    template <class Gram>
    bool scoreable_rule(Gram& gram, typename Gram::rule_type r) const
    {
        return gram.rule_has_property(r,lmstrid);
    }

 private:
    lm_id_type lm_id(indexed_token tok) const { return (*lmmap)[tok.index()]; }

    template <class Accum>
    void
    heuristic_score(info_type const& info,Accum const& accum) const; // used only with identity lmstring, or for debugging

//    typedef std::vector<const info_type* > lmkids_type;

  //FIXME: might be nice if we didn't have to copy the constituent iter to our own random access array? nah, <1% performance.
    struct lmkids
    {
        typedef info_type const* IP;

//        IP kids_end;
        IP operator[](unsigned i) const
        {
            return kids[i];
        }
        
        size_t size() const { return kids.size(); }
        
        lmkids() {}

        template <class ConstituentIterator>
        lmkids(boost::iterator_range<ConstituentIterator> rng)
        {
            ConstituentIterator itr = boost::begin(rng), end = boost::end(rng);
//            kids.reserve(end-itr);
            for (; itr != end; ++itr) {
                if (itr->root().type() != foreign_token) {
                    kids.push_back(itr->info());
                }
            }
        }
     private:
        std::vector<IP> kids;
    };
    
    template <class LMSTR>
    bool start_symbol(lmkids const& lmk,LMSTR const& lmstr) const
    {
        for (size_t x = 0; x != lmk.size(); ++x) if ((*lmk[0])(0,0) == lm.lm().start_id) return true;
        if (lmstr.begin() != lmstr.end() and lmstr.begin()->is_token() and lm_id(lmstr.begin()->get_token()) == lm.lm().start_id) return true;
        return false;
    }

    typedef lmkids lmkids_type;

    // FIXME: for now *we* manage the <s> </s> LM pseudo-words for toplevel items
    // shouldn't this be part of TOP rule lmstring? - does this make us
    // inflexible w.r.t. parsing non-sentential things?  what if the input to
    // decoder is the previous N words of a monotone chunkwise translation?
    //
    // 2009 - Michael - I buy that.  to do so, it needs to be possible for
    // the binarizer to accept an override for the lmstring of some rules.
    // for instance, if the xrs rule provides an lm_string={{{}}}, then the
    // binarizer should binarize it, instead of the yield of the rule.
    // this would also solve the implicit unk rule creation.
    template <class Gram>
    void compute_ngrams( info_type &n
                       , score_t &inside
                       , score_t &heuristic
                       , Gram& gram
                       , bool is_toplevel
                       , typename ngram_rule_data<Gram>::return_type lmstr
                       , lmkids const& lmk ) ;

    // returns inside components.  note: feat ids start at 0? not sure how names are assigned to grammar w/ those ids
    // \see FIXME above
    template <class Gram>
    boost::tuple<component_scores_vec,component_scores_vec>
    compute_ngrams( info_type &n
                  , Gram& gram
                  , bool is_toplevel
                  , typename ngram_rule_data<Gram>::return_type lmstr
                  , lmkids const& lmk ) ;

    // above are implemented by this; see .ipp
    // \see FIXME above
    template <class Gram,class Accum,class AccumH>
    void
    compute_ngrams_generic( info_type &n
                          , Accum const& accum
                          , AccumH const& accum_heuristic
                          , Gram& gram
                          , bool is_toplevel
                          , typename ngram_rule_data<Gram>::return_type lmstr
                          , lmkids const& lmk ) ;

    bool shorten;
    unsigned lmstrid;
    boost::shared_ptr< std::vector<sbmt::indexed_token::size_type> > lmmap;
};

} // namespace sbmt

#include "sbmt/edge/impl/ngram_info.ipp"

#endif
