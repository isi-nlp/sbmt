# if ! defined(SBMT__FEATURE__INDICATOR_HPP)
# define       SBMT__FEATURE__INDICATOR_HPP

/* you have something that changes names for ids. you want indicator features for pairs of ids. names will never contain [ or ] (up to you to escape). this will get you the relevant feature weights for decoding, and produce (possibly new) feature ids for component score output. optional caching of generated names to avoid repeated string ops

   of course, nothing says these have to be binary (indicator) features. optional score argument can be anything
 */


# include <algorithm>
# include <utility>
# include <sbmt/hash/oa_hashtable.hpp>
# include <sbmt/token/token.hpp>
# include <boost/tuple/tuple.hpp>
# include <string>
# include <iostream>
# include <graehl/shared/lock_policy.hpp>
# include <graehl/shared/string_tr.hpp>
# include <graehl/shared/string_match.hpp>
# include <graehl/shared/char_transform.hpp>
# include <sbmt/token/indexed_token.hpp>
# include <sbmt/feature/feature_vector.hpp>
# include <sbmt/ngram/dynamic_ngram_lm.hpp>

namespace boost { namespace tuples {

inline std::size_t hash_value(null_type const&) { return 0; }

template <class H, class T>
std::size_t hash_value(cons<H,T> const& t)
{
  std::size_t x = hash_value(t.get_tail());
  boost::hash_combine(x,t.get_head());
  return x;
}

} }

namespace sbmt {

static const score_t indicator = 0.1; // 10^-1 - cost of 1 in neglog10space.
inline score_t indicator_wt(double n)
{
  return score_t(n,as_neglog10());
}
/* token<->std::string concept:
   class with: id_type
   copyable (it has ref to whatever resource)
   a(string) => token
   a.label(token) => string
*/
template <class D>
struct str2tag {
  D *d;
  typedef str2tag<D> self_type;
  typedef typename D::token_type id_type;
  typedef id_type result_type;
  str2tag(D &d) : d(&d) {  }
  str2tag(self_type const& o) : d(o.d) {  }

  id_type operator()(std::string const& str) const
  {
    return d->tag(str);
  }
  std::string const& label(id_type t) {
    return d->label(t);
  }
};

typedef str2tag<indexed_token_factory> tag_from_dict;

template <class D>
struct str2tag_existing : str2tag<D> {
  typedef str2tag_existing<D> self_type;
  typedef str2tag<D> parent_type;
  str2tag_existing(D &d) : parent_type(d) {  }
  str2tag_existing(self_type const& o) : parent_type(o) {  }
  typename parent_type::id_type operator()(std::string const& str) const
  {
    return this->d->find_tag(str);
  }
};

typedef str2tag<indexed_token_factory> tag_from_dict_existing;

struct lm_str2id_raw {
  dynamic_ngram_lm const*d;
  typedef dynamic_ngram_lm::lm_id_type id_type;
  typedef id_type result_type;
  lm_str2id_raw(dynamic_ngram_lm const&d) : d(&d) { }
  lm_str2id_raw(lm_str2id_raw const& o) : d(o.d) {  }

  id_type operator()(std::string const& str) const
  {
    return d->id_raw_existing(str);
  }
  std::string const& label(id_type t) {
    return d->word_raw(t);
  }
};

// can print null/bo, will read then digit->@
struct lm_str2id {
  dynamic_ngram_lm const*d;
  typedef dynamic_ngram_lm::lm_id_type id_type;
  typedef id_type result_type;
  lm_str2id(dynamic_ngram_lm const& d) : d(&d) { }
  lm_str2id(lm_str2id const& o) : d(o.d) {  }

  id_type operator()(std::string const& str) const
  {
    return d->id_existing(str);
  }
  std::string const& label(id_type t) {
    return d->word(t);
  }
};

template <class D>
struct str2id {
  D *d;
  typedef str2id<D> self_type;
  typedef typename D::index_type id_type;
  typedef id_type result_type;
  str2id(D &d) : d(&d) {  }
  str2id(self_type const& o) : d(o.d) {  }

  id_type operator()(std::string const& str) const
  {
    return d->get_index(str);
  }
  std::string const& label(id_type t) {
    return d->get_token(t);
  }
};

template <class D>
struct str2id_existing {
  D const*d;
  typedef str2id_existing<D> self_type;
  typedef typename D::index_type id_type;
  typedef id_type result_type;
  str2id_existing(D const&d) : d(&d) {  }
  str2id_existing(self_type const& o) : d(o.d) {  }

  id_type operator()(std::string const& str) const
  {
    return d->get_index(str);
  }
  std::string const& label(id_type t) {
    return d->get_token(t);
  }
};

typedef str2id<in_memory_token_storage> str2id_storage;
typedef str2id_existing<in_memory_token_storage> str2id_storage_existing;

/*
//why don't square brackets work? this seems confused. we write with [->{ but don't read. maybe this is for lexical items! ok.

struct no_square_brackets { //TODO: proper escape/unescape (so you can set weights for features with [] in them properly)
inline char operator()(char c) const {
if (c=='[') return '{';
if (c==']') return '}';
return c;
}
};
*/
struct no_square_brackets : public graehl::char_transform {
  no_square_brackets() : graehl::char_transform('^') { // ^ -> ^^
    map('[','{');map(']','}');map(',','/');map(':',';'); // e.g. [ -> ^{
    map('<','(');map('>',')'); //FIXME: rexamine later.
    // i just noticed forest has ruleid<feature-vector> ... probably () are safe at that point in parsing (depends on mira).
  }
};

static no_square_brackets nsb;

template <class IO,class H,class T>
void print_ind(std::ostream &o,IO io,boost::tuples::cons<H,T> const& t) {
  std::string const& s=io.label(t.get_head());
  o<<'[';
  // graehl::write_tr(o,s,no_square_brackets());
  nsb.escape(s.begin(),s.end(),std::ostream_iterator<char>(o));
  o<<']';
  print_ind(o,io,t.get_tail());
}

template <class IO>
void print_ind(std::ostream &,IO,boost::tuples::null_type const&) {
}

template <class IO,class H,class T>
std::string::size_type read_tuples(std::string const& s,std::string::size_type p,IO io,boost::tuples::cons<H,T> & t) {
  if (s[p]=='[') {
    ++p;
    std::string::size_type q=s.find(']',p);
    if (q!=std::string::npos) {
      t.get_head()=io(nsb.unescape(s.begin()+p,s.begin()+q)); //std::string(s,p,q-p)
      p=read_tuples(s,q+1,io,t.get_tail());
      return p;
    }
  }
  throw std::runtime_error("expected sequence of [token] following feature template name, but got: '"+s+"' at: "+std::string(s,p));
}

template <class IO>
std::string::size_type read_tuples(std::string const&,std::string::size_type p,IO,boost::tuples::null_type const&) {
  return p;
}

template <class Output,
          class Namer>
struct components_out {
  typedef typename Namer::name name;
  Namer &namer;
  Output o;
  components_out(Namer &namer,Output o) : namer(namer),o(o) {  }
  void operator()(name const& n,score_t v=indicator) {
    *o=std::make_pair(namer(n),v);
    assert(!nonfinite_cost(v,"components_out"));
    ++o; // avoid postincr
  }
};

template <class Output,class Namer>
components_out<Output,Namer> make_components_out(Namer &namer,Output o) {
  return components_out<Output,Namer>(namer,o);
}


template <class Namer>
struct weight_combine {
  typedef typename Namer::name name;
  Namer & namer; // non-const because of caching/locking
  score_t &s;
  weight_combine(Namer &namer,score_t &s) : namer(namer),s(s) {  }
  void operator()(name const& n,score_t v=indicator) const {
    namer.weight_feature(s,n,v);
  }
};

template <class Namer>
weight_combine<Namer> make_weight_combine(Namer &namer,score_t &s) {
  return weight_combine<Namer>(namer,s);
}

template <class Tokenio
         ,class Tokens=boost::tuple<typename Tokenio::id_type,typename Tokenio::id_type> // this should be some kind of tuple type; there just isn't an integer-templated same-type one (maybe use boost::array<id,N> instead?) - but if you have diff types your Tokenio can just handle them (wait: would need to augments token reader/writer interface to allow specified return type overload)
         ,class Lock=graehl::spin_locking> // locking, spin_locking, no_locking
struct indicator_namer : private Lock::mutex_type {
  typedef indicator_namer<Tokenio,Tokens,Lock> self_type;
  typedef typename Lock::guard_type lock_guard;
  typedef Tokenio tokenio;
  typedef typename tokenio::id_type token;
  typedef Tokens name;
  typedef feature_names_type fnames;
  typedef std::string fname;
  //typedef boost::tuple<double,feature_id_type> featwt;
  //typedef oa_hash_map<name,featwt> name2feat;
  typedef oa_hash_map<name,feature_id_type> name2feat;
  typedef oa_hash_map<name,double> name2weight;

  indicator_namer(std::string prefix,tokenio io,fnames &fn,bool use_cache=true) : prefix(prefix),prefixb(prefix+"["),io(io),fn(fn),use_cache(use_cache) {  }
  indicator_namer(self_type const& o) : prefix(o.prefix),prefixb(o.prefixb),io(o.io),fn(o.fn),use_cache(o.use_cache),cache(o.cache),weight(o.weight) {  } // everything except the lock.

  std::string prefix,prefixb;
  void set_prefix(std::string const& prefix_) {
    prefix=prefix_;
    prefixb=prefix+"[";
  }
  tokenio io;
  fnames &fn;
  bool use_cache;
  name2feat cache;
  name2weight weight;

  typedef typename name2feat::const_iterator cache_it;
  typedef typename name2weight::const_iterator weight_it;

  feature_id_type compute_id(name const& n) const {
    return fn.get_index(name2str(n));
  }

  graehl::char_transform quote;
  fname name2str(name const& n) const {
    std::ostringstream o(prefix,std::ios_base::app);
    print(o,n);
    return o.str();
  }
  template <class O>
  void print(O &o,name const &n) const {
    print_ind(o,io,n);
  }
  bool str2name(std::string const& s,name &ret) const {
    if (graehl::starts_with(s,prefixb)) {
      read_tuples(s,prefix.size(),io,ret);
      return true;
    } else
      return false;
  }


  feature_id_type operator()(name const& n) { // non-const because of cache
    if (use_cache) {
      lock_guard l(*this);
      cache_it i=cache.find(n);
      if (i!=cache.end()) return i->second/*.get<1>()*/;
    }
    feature_id_type id=compute_id(n);
    if (use_cache) {
      lock_guard l(*this);
      cache[n]/*.get<1>()*/=id;
    }
    return id;
  }
  /* // no locking; assumed that id calls happen nonoverlapping with accum */
  void weight_feature(score_t &s,name const& n,score_t val) {
    weight_it i=weight.find(n);
    if (i!=weight.end())
      s*=pow(val,i->second/*.get<0>()*/);
  }

  void set_weight(name const& n,double wt) {
    if (wt)
      weight[n]=wt;
    else
      weight.erase(n);
  }
  void set_weights(weight_vector const& w) {
    name n;
    for (weight_vector::const_iterator i=w.begin(),e=w.end();i!=e;++i)
      if (str2name(fn[i->first],n))
        set_weight(n,i->second);
  }

/*  fname const& label()(feature_id_type id) const { // not really needed
    return fn.get_token[id];
    }
*/
  typedef weight_combine<self_type> accum;
};

template <class Namer,class C, class T>
void print(std::basic_ostream<C,T>& o, typename Namer::name const &n, Namer const& namer) {
  namer.print(o,n);
}

} // namespace sbmt

# endif //     SBMT__FEATURE__INDICATOR_HPP
