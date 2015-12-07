# include <sbmt/edge/any_info.hpp>
# include <string>
# include <iostream>
# include <algorithm>
# include <boost/regex.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/function_output_iterator.hpp>
# include <sbmt/grammar/grammar_in_memory.hpp>
# include <sbmt/hash/oa_hashtable.hpp>
# include <sbmt/feature/feature_vector.hpp>
# include <gusc/generator/single_value_generator.hpp>
/*
namespace std {
template <class T, class U, class W, class V>
bool operator< (std::pair<T,U> const& p1, std::pair<W,V> const& p2)
{
    return (p1.first < p2.first) or ((not (p2.first < p1.first)) and (p1.second < p2.second));
}
} // namespace std
*/
namespace gusc {
    template <class T, class U>
    bool operator< (sparse_vector<T,U> const& s1, sparse_vector<T,U> const& s2)
    {
        return std::lexicographical_compare(s1.begin(),s1.end(),s2.begin(),s2.end());
    }
}

namespace boost { namespace tuples {

inline std::size_t hash_value(null_type const&) { return 0; }

template <class H, class T>
std::size_t hash_value(cons<H,T> const& t)
{
    std::size_t x = hash_value(t.get_tail());
    boost::hash_combine(x,t.get_head());
    return x;
}

} } // boost::tuples

using namespace sbmt;

namespace rvd {

typedef sbmt::weight_vector fvdist;
typedef boost::shared_ptr<fvdist> fvdist_ptr;

template <class Dict>
struct use_tag_index {
    boost::uint32_t operator()(std::string const& str) const
    {
        return dict->native_word(str).index();
    }
    use_tag_index(Dict& dict) : dict(&dict) {}
    Dict* dict;
};

template <class Dict>
fvdist_ptr create_fvdist(std::string const& s, Dict& dict)
{
    fvdist_ptr ret(new fvdist());
    gusc::detail::read(s,*ret,use_tag_index<Dict>(dict));
    return ret;
}

struct read_froot {
    typedef fvdist_ptr result_type;
    template <class Dictionary>
    result_type operator()(Dictionary& dict, std::string const& str) const
    {
        //std::cerr << "$$$$ " << str << '\n';
        return create_fvdist(str,dict);
    }
};

typedef std::vector<fvdist_ptr> fvars;
struct read_fvars {
    typedef fvars result_type;
    template <class Dictionary>
    result_type operator()(Dictionary& dict, std::string const& str) const
    {
        //std::cerr << "%%%% "<< str << '\n';
        std::stringstream sstr(str);
        result_type res;
        std::string lbl;
        while (sstr >> lbl) {
            res.push_back(create_fvdist(lbl,dict));
        }
        return res;
    }
};

typedef std::vector<bool> fv_indicators;
struct read_fv_indicators {
    typedef fv_indicators result_type;
    template <class Dictionary>
    result_type operator()(Dictionary& dict, std::string const& str) const
    {
        std::stringstream sstr(str);
        result_type res;
        std::copy( std::istream_iterator<bool>(sstr)
                 , std::istream_iterator<bool>()
                 , std::back_inserter(res)
                 )
                 ;
        return res;
    }
};

static score_t indicator = 0.1;

struct info_type_ {
    std::vector<fvdist_ptr> v;
    info_type_() {}
    explicit info_type_(fvdist_ptr const& p) : v(1,p) {}
};

bool operator == (info_type_ const& v1, info_type_ const& v2)
{
    if (v1.v.size() != v2.v.size()) return false;
    size_t x = 0;
    for (; x != v1.v.size(); ++x) {
        if (*(v1.v[x]) != *(v2.v[x])) return false;
    }
    return true;
}

bool operator != (info_type_ const& v1, info_type_ const& v2)
{
    return !(v1 == v2);
}

size_t hash_value(info_type_ const& v)
{
    size_t ret = 0;
    BOOST_FOREACH(fvdist_ptr const& p,v.v) {
        boost::hash_combine(ret,*p);
    }
    return ret;
}


class info_factory 
: public sbmt::info_factory_new_component_scores<info_factory>
{
public:
    typedef info_type_ info_type;
    typedef boost::tuple<info_type,sbmt::score_t,sbmt::score_t> result_type;
    typedef gusc::single_value_generator<result_type> result_generator;
    typedef sbmt::grammar_in_mem grammar;
private:
    boost::uint32_t froot_name;
    boost::uint32_t fvars_name;
    boost::uint32_t fv_indicators_name;
    boost::uint32_t hits_id;
    boost::uint32_t misses_id;
    boost::uint32_t wild_id;
    double hits_wt;
    double misses_wt;
    double wild_wt;
    info_type missing;

    template <class Rng>
    boost::tuple<info_type,double,double,double>
    build_(grammar const& g, grammar::rule_type r, Rng children, bool print=false) const
    {
        double hits = 0.0;
        double misses = 0.0;
        double wild = 0.0;
        info_type retvec;
        fv_indicators const* fvind = 0;
        int fvindidx = 0;
        if (g.rule_has_property(r,fv_indicators_name)) {
            fvind = &g.rule_property<fv_indicators>(r,fv_indicators_name);
        }
        BOOST_FOREACH(typename boost::range_value<Rng>::type const& v, children) {
            if (is_native_tag(v.root())) {
                if (fvind and fvind->at(fvindidx++)) {
                    retvec.v.insert(retvec.v.end(),v.info()->v.begin(),v.info()->v.end());
                } else {
                    wild += 1.0;
                }
            } else if (is_virtual_tag(v.root())) {
                retvec.v.insert(retvec.v.end(),v.info()->v.begin(),v.info()->v.end());
            }
        }
        //if (print) {
            //std::cerr << ">> rectvec.size=" << retvec.size() << " {";
          //  BOOST_FOREACH(fvdist_ptr ff, retvec) {
        //        std::cerr << sbmt::print(*ff,g.dict().tag_token_factory()) << ' ';
         //   }
            //std::cerr << "}\n";
        //}
        if (g.is_complete_rule(r)) {
            //if (print) std::cerr << ">> complete-rule-check\n";
            if (g.rule_has_property(r,fvars_name)) {
                fvars const& rv = g.rule_property<fvars>(r,fvars_name);
                //if (print) {
                //    std::cerr << ">> rv.size="<< rv.size() << " {";
                //    BOOST_FOREACH(fvdist_ptr ff, rv) {
                //        std::cerr << sbmt::print(*ff,g.dict().tag_token_factory()) << ' ';
                //    }
                //    std::cerr <<"}";
                //}
                int counter = 0;
//                assert(rv.size() == retvec.size());
                BOOST_FOREACH(fvdist_ptr vv, retvec.v) {
                    double h = 0.0;
                    double m = 0.0;
                    BOOST_FOREACH(fvdist::value_type vt,*vv) {
                        BOOST_FOREACH(fvdist::value_type wt, *(rv[counter])) {
                            if (vt.first == wt.first) h += vt.second * wt.second;
                            else m += vt.second * wt.second;
                        }
                    }
                    hits += h;
                    misses += m;
                    wild += (1.0 - (h + m));
                    ++counter;
                }
            } else {
                //if (print) std::cerr << ">> no fvars feature (" << fvars_name <<")\n";
            }
            if (g.rule_has_property(r,froot_name)) {
                return boost::make_tuple(
                         info_type(g.rule_property<fvdist_ptr>(r,froot_name))
                       , hits
                       , misses
                       , wild
                       );
            } else {
                return boost::make_tuple(missing,hits,misses,wild);
            }
        }
        return boost::make_tuple(retvec,hits,misses,wild);
    }

public:
    bool scoreable_rule(grammar const& g, grammar::rule_type r) const
    {
        return true;
    }

    sbmt::score_t rule_heuristic( grammar const& g
                                , grammar::rule_type r ) const
    {
        return 1.0;
    }

    std::string hash_string( grammar const& g
                           , info_type const& i ) const
    {
        return "FIXME";
    }

    template <class Range>
    result_generator
    create_info( grammar const& g
               , grammar::rule_type r
               , span_t const&
               , Range children ) const
    {
        double hits, misses, wild;
        info_type ret;
        boost::tie(ret,hits,misses,wild) = build_(g,r,children);
        return boost::make_tuple(
                 ret
               , sbmt::pow(indicator,hits*hits_wt) *
                 sbmt::pow(indicator,misses*misses_wt) *
                 sbmt::pow(indicator,wild*wild_wt)
               , score_t(1.0)
               );
    }

    template <class Range, class Output>
    Output component_scores_old( grammar& g
                               , grammar::rule_type r
                               , sbmt::span_t const&
                               , Range children
                               , info_type const&
                               , Output out ) const
    {
        double hits,misses,wild;
        info_type ret;
        boost::tie(ret,hits,misses,wild) = build_(g,r,children,true);
        *out = std::make_pair(hits_id,sbmt::pow(indicator,hits));
        ++out;
        *out = std::make_pair(misses_id,sbmt::pow(indicator,misses));
        ++out;
        *out = std::make_pair(wild_id,sbmt::pow(indicator,wild));
        ++out;
        //std::cerr << sbmt::print(r,g) <<" match="<<hits<<" nomatch="<<misses<<" missing="<<wild<<'\n';
        return out;
    }

    info_factory( grammar& g
                , boost::uint32_t r
                , boost::uint32_t v
                , boost::uint32_t i
                , std::string p )
      : froot_name(r)
      , fvars_name(v)
      , fv_indicators_name(i)
      , hits_id(g.feature_names().get_index(p+"_match"))
      , misses_id(g.feature_names().get_index(p+"_nomatch"))
      , wild_id(g.feature_names().get_index(p+"_missing"))
      , hits_wt(get(g.get_weights(),g.feature_names(),p+"_match"))
      , misses_wt(get(g.get_weights(),g.feature_names(),p+"_nomatch"))
      , wild_wt(get(g.get_weights(),g.feature_names(),p+"_missing"))
      , missing(fvdist_ptr(new fvdist()))
      {
//          assert(missing.size() == 1);
      }
};

struct constructor;

struct setter {
    enum type {prefix, root, vars, indicator};
    constructor* cons;
    type t;
    bool is_set;
    setter() : cons(NULL), t(prefix), is_set(false) {}
};

std::istream& operator >> (std::istream& in, setter& s);
std::ostream& operator << (std::ostream& out, setter const& s);

struct constructor {
    std::string name;
    std::string prefix;
    std::string root;
    std::string vars;
    std::string indicator;
    setter ps, rs, vs, is;

    constructor(std::string name) : name(name)
    {
        ps.cons = this; ps.t = setter::prefix;
        vs.cons = this; vs.t = setter::vars;
        rs.cons = this; rs.t = setter::root;
        is.cons = this; is.t = setter::indicator;
        prefix = name;
        root = name + "_root";
        vars = name + "_vars";
        indicator = name + "_var_indicators";
    }

    void swap_self(constructor& o)
    {
        swap(name,o.name);
        swap(prefix,o.prefix);
        swap(root,o.root);
        swap(vars,o.vars);
        swap(indicator,o.indicator);
        std::swap(ps.is_set,o.ps.is_set);
        std::swap(rs.is_set,o.rs.is_set);
        std::swap(vs.is_set,o.vs.is_set);
        std::swap(is.is_set,o.is.is_set);
    }

    constructor(constructor const& o)
    : name(o.name)
    , prefix(o.prefix)
    , root(o.prefix)
    , vars(o.vars)
    , ps(o.ps)
    , rs(o.rs)
    , vs(o.vs)
    , is(o.is)
    {
        ps.cons = this;
        vs.cons = this;
        rs.cons = this;
        is.cons = this;
    }

    constructor& operator=(constructor const& o)
    {
        constructor(o).swap_self(*this);
        return *this;
    }

    sbmt::options_map get_options()
    {
        sbmt::options_map opts("root-variable dist");
        opts.add_option( name + "-prefix-name"
                       , sbmt::optvar(ps)
                       , "prefix for feature names, ie ${prefix}_match, $prefix[x][y], etc.\n"
                         "default=" + name
                       );
        opts.add_option( name + "-root-name"
                       , sbmt::optvar(rs)
                       , "name of root feature attached to rules.\n"
                         "default=${prefix}_root"
                       );
        opts.add_option( name + "-vars-name"
                       , sbmt::optvar(vs)
                       , "name of variable list feature attached to rules.\n"
                         "default=${prefix}_vars"
                       );
        opts.add_option( name + "-var-indicators-name"
                       , sbmt::optvar(is)
                       , "name of variable list feature attached to rules.\n"
                         "default=${prefix}_var_indicators"
                       );
        return opts;
    }
    void init(sbmt::in_memory_dictionary& dict) {}

    sbmt::any_info_factory construct( sbmt::grammar_in_mem& grammar
                                    , sbmt::lattice_tree const& lat
                                    , sbmt::property_map_type pmap )
    {
        //fvdist_set.clear();
        std::cerr << "prefix=" << prefix << '\n'
                  << "root-name=" << root << " -> "<< pmap[root] << '\n'
                  << "vars-name=" << vars << " -> "<< pmap[vars] << '\n'
                  << "indicator-name="<< indicator << " -> " << pmap[indicator] << '\n';
        return info_factory(grammar,pmap[root],pmap[vars],pmap[indicator],prefix);
    }

    bool set_option(std::string const& nm, std::string const& vl)
    {
        return false;
    }
};

std::ostream& operator << (std::ostream& out, setter const& s)
{
    if (s.t == setter::prefix) return out << s.cons->prefix;
    if (s.t == setter::root) return out << s.cons->root;
    if (s.t == setter::vars) return out << s.cons->vars;
    if (s.t == setter::indicator) return out << s.cons->indicator;
    return out;
}

std::istream& operator >> (std::istream& in, setter& s)
{
    std::string str;
    in >> str;
    s.is_set = true;

    unregister_rule_property_constructor(s.cons->name,s.cons->root);
    unregister_rule_property_constructor(s.cons->name,s.cons->vars);
    unregister_rule_property_constructor(s.cons->name,s.cons->indicator);

    if (s.t == setter::prefix) {
        s.cons->prefix = str;
        if (s.cons->rs.is_set == false) s.cons->root = str + "_root";
        if (s.cons->vs.is_set == false) s.cons->vars = str + "_vars";
        if (s.cons->is.is_set == false) s.cons->indicator = str + "_var_indicators";
    } else if (s.t == setter::root) {
        s.cons->root = str;
    } else if (s.t == setter::vars) {
        s.cons->vars = str;
    } else if (s.t == setter::indicator) {
        s.cons->indicator = str;
    }

    register_rule_property_constructor(s.cons->name,s.cons->root,read_froot());
    register_rule_property_constructor(s.cons->name,s.cons->vars,read_fvars());
    register_rule_property_constructor(s.cons->name,s.cons->indicator,read_fv_indicators());

    return in;
}

struct rvdinit {
    rvdinit()
    {
        # define BOOST_PP_LOCAL_LIMITS (0,9)
        # define BOOST_PP_LOCAL_MACRO(N) \
        register_info_factory_constructor( "rvd" BOOST_PP_STRINGIZE(N) \
                                         , constructor("rvd" BOOST_PP_STRINGIZE(N)) \
                                         ); \
        register_rule_property_constructor( "rvd" BOOST_PP_STRINGIZE(N) \
                                          , "rvd" BOOST_PP_STRINGIZE(N) "_root" \
                                          , read_froot() \
                                          ); \
        register_rule_property_constructor( "rvd" BOOST_PP_STRINGIZE(N) \
                                          , "rvd" BOOST_PP_STRINGIZE(N) "_vars" \
                                          , read_fvars() \
                                          ); \
        register_rule_property_constructor( "rvd" BOOST_PP_STRINGIZE(N) \
                                          , "rvd" BOOST_PP_STRINGIZE(N) "_var_indicators" \
                                          , read_fv_indicators() \
                                          );
        # include BOOST_PP_LOCAL_ITERATE()
    }
};

rvdinit rvdinit_;

} // namespace ssyn
