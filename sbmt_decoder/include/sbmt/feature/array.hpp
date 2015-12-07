# if ! defined(SBMT__FEATURE__ARRAY_HPP)
# define       SBMT__FEATURE__ARRAY_HPP

/* you have N (known in advance) features named prefix1 ... prefixN
 */


# include <algorithm>
# include <utility>
# include <sbmt/hash/oa_hashtable.hpp>
# include <sbmt/token/token.hpp>
# include <string>
# include <iostream>
# include <sbmt/token/indexed_token.hpp>
# include <sbmt/feature/feature_vector.hpp>
# include <graehl/shared/dynamic_array.hpp>

namespace sbmt {

struct arrayfeats {
  typedef unsigned name;
  typedef boost::uint32_t fid_t;
  typedef double fwt_t;
  typedef graehl::dynamic_array<fid_t> fids_t;
  typedef graehl::dynamic_array<fwt_t> fwts_t;
  typedef feature_names_type fnames;
  typedef std::string fname;
  fnames &fn;
  name N;
  fids_t fids;
  fwts_t fwts;
  arrayfeats(fname &fn,name N=1) : fn(fn),N(N),fids(N,0),fwts(N,0) { // call set_weights
  }

  fname name2str(name i) {
    std::ostringstream o(prefix,std::ios_base::app);
    o<<i;
    return o.str();
  }
  feature_id_type compute_id(name const& n) const {
    return fn.get_index(name2str(n));
  }
  void set_weights(weight_vector const& w) {
    for (name i=0;i<N;++i)
      fwts[i]=w[fids[i]=compute_id(i+1)]; // named 1...N not 0...
  }

# if 0
  typedef boost::tuple<fid_t,fwt_t> fidwt_t;
  typedef graehl::dynamic_array<fidwt_t> fidwts_t;
  fidwts_t fidwts;
# endif
};


template <class Output>
struct array_components_out {
  typedef arrayfeats Namer;
  typedef Namer::name name;
  arrayfeats::fids_t &namer;
  Output o;
  components_out(arrayfeats::fids_t &namer,Output o) : namer(namer),o(o) {  }
  void operator()(name n,score_t v=array) {
    *o=std::make_pair(boost::get<0>(namer[n]),v);
    assert(!nonfinite_cost(v,"components_out"));
    ++o; // avoid postincr
  }
};

template <class Output>
components_out<Output> make_array_components_out(arrayfeats const& namer,Output o) {
  return array_components_out<Output>(namer.fids,o);
}


struct array_weight_combine {
  typedef arrayfeats Namer;
  typedef typename Namer::name name;
  arrayfeats::fwts_t const& namer;
  score_t &s;
  array_weight_combine(arrayfeats::fwts_t &namer,score_t &s) : namer(namer),s(s) {  }
  void operator()(name n,score_t v=array) const {
    s*=pow(v,namer[n]);
  }
};

array_weight_combine make_array_weight_combine(arrayfeats::fidwts_t const& namer,score_t &s) {
  return array_weight_combine(namer.fwts,s);
}


} // namespace sbmt

# endif //     SBMT__FEATURE__ARRAY_HPP
