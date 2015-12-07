# if ! defined(SBMT__FEATURE_VECTOR_HPP)
# define       SBMT__FEATURE_VECTOR_HPP

# include <graehl/shared/word_spacer.hpp>
# include <gusc/functional.hpp>
# include <gusc/math/sparse_vector.hpp>
# include <sbmt/logmath.hpp>
# include <sbmt/token/in_memory_token_storage.hpp>
# include <sbmt/logging.hpp>

namespace gusc {

template <class T, class L, class F>
sparse_vector< T, sbmt::basic_lognumber<L,F> >&
operator*=( sparse_vector< T, sbmt::basic_lognumber<L,F> >& v1
          , sparse_vector< T, sbmt::basic_lognumber<L,F> > const& v2
          )
{
    sparse_op_inplace(v1,v2,times());
    return v1;
}

template <class T, class L, class F>
sparse_vector< T, sbmt::basic_lognumber<L,F> >
operator*( sparse_vector< T, sbmt::basic_lognumber<L,F> > const& v1
         , sparse_vector< T, sbmt::basic_lognumber<L,F> > const& v2 )
{
  sparse_vector< T, sbmt::basic_lognumber<L,F> > w;
  sparse_op_copy(w,v1,v2,times());
  return w;
}

template <class T, class L, class F>
sparse_vector< T, sbmt::basic_lognumber<L,F> >&
operator/=( sparse_vector< T, sbmt::basic_lognumber<L,F> >& v1
          , sparse_vector< T, sbmt::basic_lognumber<L,F> > const& v2 )
{
  sparse_op_inplace(v1,v2, divide());
  return v1;
}

template <class T, class L, class F>
sparse_vector< T, sbmt::basic_lognumber<L,F> >
operator/( sparse_vector< T, sbmt::basic_lognumber<L,F> > const& v1
         , sparse_vector< T, sbmt::basic_lognumber<L,F> > const& v2 )
{
  sparse_vector< T, sbmt::basic_lognumber<L,F> > w;
  sparse_op_copy(w,v1,v2,divide());
  return w;
}

template <class L, class F>
struct reverse_pow {
    typedef sbmt::basic_lognumber<L,F> result_type;

    sbmt::basic_lognumber<L,F>
    operator()(double x, sbmt::basic_lognumber<L,F> const& y) const
    {
        return pow(y,x);
    }
};

template <class T, class L, class F>
sbmt::basic_lognumber<L,F>
geom( sparse_vector< T, sbmt::basic_lognumber<L,F> > const& v
    , sparse_vector<T,double> const& w )
{
    return sparse_op_reduce(w, v, reverse_pow<L,F>(),times());
}

namespace detail {
template <class Dict>
struct use_index {
    boost::uint32_t operator()(std::string const& str) const
    {
        return dict->get_index(str);
    }
    use_index(Dict& dict) : dict(&dict) {}
    Dict* dict;
};
} // sbmt::detail

template <class T, class V, class Dict>
void read(sparse_vector<T,V>& v, std::string const& str, Dict& dict)
{
    detail::read(str,v,detail::use_index<Dict>(dict));
}

template <class K, class V, class S, class T, class Dict>
void print(std::basic_ostream<S,T>& os, sparse_vector<K,V> const& v, Dict const& dict, bool print_zerocost=false, char kvsep=':',char pairsep=',',V const &zero=V())
{
    typename sparse_vector<K,V>::const_iterator
        itr = boost::begin(v),
        end = boost::end(v);
    graehl::word_spacer sp(pairsep);
    for (; itr != end; ++itr) {
      if (print_zerocost || itr->second!=zero)
        os << sp << dict.get_token(itr->first) << kvsep << itr->second;
    }
}

template <class V, class Dict>
gusc::sparse_vector<boost::uint32_t,V>
index(gusc::sparse_vector<std::string,V> const& v, Dict& dict)
{
    gusc::sparse_vector<boost::uint32_t,V> f;
    typedef typename gusc::sparse_vector<std::string,V>::value_type v_t;
    BOOST_FOREACH(v_t p, v) {
        f[dict.get_index(p.first)] = p.second;
    }
    return f;
}

} // gusc

namespace sbmt {
SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(feature_domain,"features",root_domain);

typedef in_memory_token_storage feature_names_type;
typedef boost::uint32_t feature_id_type; //feature_names_type::index_type
typedef gusc::sparse_vector<feature_id_type,score_t> feature_vector;
typedef gusc::sparse_vector<feature_id_type,double> weight_vector;
typedef gusc::sparse_vector<std::string,score_t> fat_feature_vector;
typedef gusc::sparse_vector<std::string,double> fat_weight_vector;
typedef in_memory_token_storage feature_dictionary;
using gusc::geom;
using gusc::read;
using gusc::print;

template <class Score,class Name>
bool nonfinite_cost(Score const& s,Name const& name,bool log=true,char const* prefix="") {
    bool nonf=!s.is_positive_finite();
    if (nonf && log)
      SBMT_WARNING_STREAM(feature_domain,prefix<<name<<'='<<s<<" cost is not finite (i.e. prob is not positive and finite) - tuning and 1best may be ill-defined!");
    return nonf;
}

template <class V,class TF>
unsigned check_nonfinite_costs(V const& v,TF const& tf,bool log=true,char const* prefix="nbest: ") {
    unsigned n=0;
    for (typename V::const_iterator i=v.begin(),e=v.end();i!=e;++i) {
      //std::string const& f=tf.get_token(i->first);
      n+=nonfinite_cost(i->second,tf.get_token(i->first));
    }
    return n;
}


} // namespace sbmt

# endif //     SBMT__FEATURE_VECTOR_HPP
