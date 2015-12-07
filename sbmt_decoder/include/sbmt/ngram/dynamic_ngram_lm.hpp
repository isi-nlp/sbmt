#ifndef SBMT_NGRAM__DYNAMIC_NGRAM_LM_HPP
#define SBMT_NGRAM__DYNAMIC_NGRAM_LM_HPP

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem/operations.hpp>
#include <sbmt/ngram/cached_ngram_lm.hpp>
#include <sbmt/ngram/base_ngram_lm.hpp>
#include <gusc/generator/arithmetic_generator.hpp>

namespace sbmt {

//        first_open_id = bo_id+1,  //LW::LWVocab::INVALID_WORD,  //LW::LWVocab::NONE

struct dynamic_ngram_lm;

/** description:

lmname1=lw[@,c=1e-20][filename_without_paren or starting quotes]
lmname2=big[]["filename with quotes but ]_[ allowed"]

(c=x means closed lm, p(<unk>)=x)

or multilm:

name=multi[c=10^-10][sub1=lw[][file],sub2=big[@][file2]]

*/
    /// must delete yourself (e.g. boost::shared_ptr)

typedef boost::shared_ptr<dynamic_ngram_lm> ngram_ptr;


struct ngram_lm_builder_impl
{
    virtual ngram_ptr build(ngram_options const& opt,std::istream &i,boost::filesystem::path const& relative_base,unsigned cache_size) = 0;
    virtual ~ngram_lm_builder_impl() {}
};


template <class LM>
struct ngram_lm_builder : public ngram_lm_builder_impl
{
    ngram_ptr build(ngram_options const& opt,std::istream &i,boost::filesystem::path const& relative_base,unsigned cache_size)
    {
        if (cache_size==0)
            return ngram_ptr(new LM(opt,i,relative_base));
        return ngram_ptr(new cached_ngram_lm<LM>(opt,i,relative_base,cache_size));
    }
};

/// this is the singleton you use to build dynamic ngram LMs.  note cache_size=0 means no caching.  cache_size > than that gives you caching of only prob queries, not suffix/prefix/backoff
struct ngram_lm_factory
{
//    template <class  LM> friend class ngram_lm_register<LM>;
    static ngram_lm_factory &instance()
    {
        return s_instance;
    }
    ngram_ptr create(std::istream &i,boost::filesystem::path const& relative_base=boost::filesystem::initial_path(),unsigned cache_size=0);
    ngram_ptr create(std::string const &spec,boost::filesystem::path const& relative_base=boost::filesystem::initial_path(),unsigned cache_size=0);
// private:
    typedef boost::shared_ptr<ngram_lm_builder_impl> Builder;
    std::map<std::string,Builder> maker;
    static ngram_lm_factory s_instance;
};

template <class LM>
struct ngram_lm_register
{
    typedef boost::shared_ptr<ngram_lm_builder_impl> Builder;
    ngram_lm_register(std::string const& name)
    {
        ngram_lm_factory::instance().maker[name]=Builder(new ngram_lm_builder<LM>());
    }
};


struct dynamic_ngram_lm : public fat_ngram_lm<dynamic_ngram_lm,unsigned>, boost::noncopyable
{
 protected:
 public:
    typedef fat_ngram_lm<dynamic_ngram_lm,unsigned> Fat;
    typedef unsigned lm_id_type;
    typedef lm_id_type * iterator;
    typedef lm_id_type const* const_iterator;

/// note: ensure that these constant ids don't collide with the ids your vocab uses; if this means shifting your origin, sorry
//    BOOST_STATIC_CONSTANT(lm_id_type,null_id=(lm_id_type)-1);
//    BOOST_STATIC_CONSTANT(lm_id_type,bo_id=(lm_id_type)-2); // intended to use in last slot of left context, for prematurely shortened but needing more backoffs
    enum {
        null_id=-1,bo_id=-2
    };

    /// all of the following methods must be defined:

    lm_id_type unk_id,start_id,end_id; // you must initialize these

  typedef gusc::any_generator<lm_id_type> vocab_generator;
  typedef gusc::arithmetic_generator<lm_id_type> vocab_range_generator;
  virtual vocab_generator vocab() const = 0;

  virtual lm_id_type first_unused_id(unsigned consecutive=1<<20) const { // give an id that has free id subspace [id,id+consecutive) - i.e. max observed id + 1, excluding <s> </s> UNK NULL BO (but avoiding them if necessary). note: these ids aren't added to vocab, so adding any further words would cause conflicts, and you can't get a string for them.
    vocab_generator g=vocab();
    lm_id_type max=0;
    while(g) {
      lm_id_type i=g();
      bool is_special = i==(lm_id_type)null_id || i==(lm_id_type)bo_id || i==unk_id || i==start_id || i==end_id;
      if (i>max && (i-max<consecutive || !is_special))
        max=i;
    }
    return max+1;
  }

    virtual void stats(std::ostream &o) const {
    }

    virtual void set_weights_raw(weight_vector const& weights, feature_dictionary& dict);

    virtual void describe(std::ostream &o) const;

    virtual score_t print_sequence_details_raw(std::ostream &o,const_iterator history_start,const_iterator score_start,const_iterator end,bool factors=false) const
    {
        return Fat::print_sequence_details_raw(o,history_start,score_start,end,factors); // calls print_further already
    }

    /// [history_start,score_start,end): \prod{s in [score_start,end)} p(s|history_start...s-1)
    virtual void print_further_sequence_details(std::ostream &o,const_iterator history_start,const_iterator score_start,const_iterator end,bool factors=false) const;

    /// (don't worry about digit->@ replacement, it's already done).

    ///already done for you: "#"<->bo_id, "*"<->null_id

    /// you provide: regular vocab<->id, "<s>" <-> start_id, "</s>" <-> end_id, "<unk>" <-> unk_id.
  lm_id_type id_raw_existing(const std::string &word) const {
    return const_cast<dynamic_ngram_lm *>(this)->id_raw(word,false);
  }

    virtual lm_id_type id_raw(const std::string &word,bool add_unk=false)  = 0; /// should return unk_id if word is novel. doesn't do digit->@ (id() does). should be const if add_unk is false

    virtual std::string const& word_raw(lm_id_type id) const = 0;

    /// if ctx_and_w points to [a b c d], and len = 3, then return p(c|a,b).  if len=0, return p()=1
    virtual score_t open_prob_len_raw(const_iterator ctx_and_w,unsigned len) const = 0;

    /// (for up to max_order()-1 length phrases, return infinite cost, i.e. score_t(0), if phrase is unknown, bow(new word|phrase) otherwise
    virtual score_t find_bow_raw(const_iterator ctx,const_iterator end) const = 0;

    /// (for up to max_order() length phrases, return infinite cost, i.e. score_t(0), if phrase is unknown, p(final word|previous words of phrase) otherwise
    virtual score_t find_prob_raw(const_iterator ctx,const_iterator end) const = 0;

    /// returns N for an N-gram lm
    virtual unsigned max_order_raw() const = 0;

    /// may override: returns r such that [b,r) is the longest known history (i.e. length maxorder-1 max)
    virtual const_iterator longest_prefix_raw(const_iterator i,const_iterator end) const;

    /// may override: reverse of longest_prefix (returns r such that [r,end) is the longest seen history)
    virtual const_iterator longest_suffix_raw(const_iterator i,const_iterator end) const;

    virtual ~dynamic_ngram_lm();
    virtual bool loaded() const;
    
    virtual bool set_option(std::string,std::string) = 0;
    
    virtual Fat::feature_t* sequence_prob_array(Fat::feature_t* v,const_iterator seq,const_iterator i,const_iterator end) const
    {
        return Fat::sequence_prob_array(v,seq,i,end);
    }

    virtual Fat::feature_t* bow_interval_array(Fat::feature_t* v,const_iterator i,const_iterator e,const_iterator backoff_end) const
    {
        return Fat::bow_interval_array(v,i,e,backoff_end);
    }


    virtual void set_ids() /// call after loading lm since vocab object needs to exist. ngram_lm_factory::create does this (see .cpp)
    {
        unk_id=id_raw("<unk>",true);
        start_id=id_raw("<s>",true);
        end_id=id_raw("</s>",true);
    }
 protected:
    void finish()
    {
        //         set_ids(); // NO - can't do this for multi_ngram_lm because tf isn't known yet - do it in update_token_factory
        Fat::finish();
    }

    dynamic_ngram_lm(std::string const& type,ngram_options const& opt=ngram_options()) : Fat(type,opt) {}

};


struct load_lm //implemented in ngram_constructor.cpp and not dynamic_ngram_lm.cpp so it can use ngram_constructor logging domain.
{
    dynamic_ngram_lm *plm; // = splm.get()
    std::string spec;
    ngram_ptr splm;
    bool is_null() const { return !plm; }
    void set_null() { splm.reset();plm=NULL; }
    dynamic_ngram_lm  &lm() const { return *plm; }
    load_lm() : spec("none") {  }
    load_lm(ngram_ptr splm) : splm(splm) {
        plm=splm.get();
        spec=plm?plm->description():"none";
    }
  load_lm(load_lm const& o) : plm(o.plm),spec(o.spec),splm(o.splm) {  }
    bool load(unsigned cache_size,unsigned req_order=0);
};

#define SBMT_REGISTER_NGRAM_LM(lw_ngram_lm,lw) \
namespace { \
ngram_lm_register <lw_ngram_lm> anon_lw_register ## lw (#lw); \
}


}


#endif
