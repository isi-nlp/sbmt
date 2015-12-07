# ifndef SOURCE_SYNTAX__SOURCE_SYNTAX_HPP
# define SOURCE_SYNTAX__SOURCE_SYNTAX_HPP

# include <vector>
# include <string>
# include <sstream>
# include <iostream>
# include <sbmt/hash/oa_hashtable.hpp>
# include <gusc/generator/single_value_generator.hpp>
# include <sbmt/feature/indicator.hpp>
# include <boost/regex.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/function_output_iterator.hpp>
# include <sbmt/edge/any_info.hpp>

namespace ssyn {

struct read_froot {
    typedef sbmt::indexed_token result_type;
    template <class Dictionary>
    result_type operator()(Dictionary& dict, std::string const& str) const
    {
        return dict.tag(str);
    }
};

typedef std::vector<sbmt::indexed_token> fvars;
struct read_fvars {
    typedef fvars result_type;
    template <class Dictionary>
    result_type operator()(Dictionary& dict, std::string const& str) const
    {
        std::stringstream sstr(str);
        result_type res;
        std::string lbl;
        while (sstr >> lbl) {
            res.push_back(dict.tag(lbl));
        }
        return res;
    }
};

template <class Grammar>
struct source_vars {
    typedef fvars type;
};

struct feat_name {
    enum type {FULL = 0, MATCH = 1, NOMATCH = 2, MISSING = 3, ST = 4};
    struct missing_ {}; static missing_ missing;
    struct match_ {}; static match_ match;
    struct nomatch_ {}; static nomatch_ nomatch;
    struct st_ {}; static st_ st;
    struct full_ {}; static full_ full;
    
    type t;
    boost::tuple<sbmt::indexed_token,sbmt::indexed_token> n;

    feat_name(missing_) : t(MISSING) {}
    feat_name(nomatch_) : t(NOMATCH) {}
    feat_name(match_) : t(MATCH) {}
    feat_name(full_, sbmt::indexed_token x, sbmt::indexed_token y) : t(FULL), n(x,y) {}
    feat_name(st_, sbmt::indexed_token x, sbmt::indexed_token y) : t(ST), n(x,y) {}
    bool operator==(feat_name const& o) const
    {
        return o.t == t and ((t == FULL or t == ST) ? o.n == n : true);
    }

    sbmt::indexed_token get0() const { return n.get<0>(); }
    sbmt::indexed_token get1() const { return n.get<1>(); }

    size_t hash_value() const
    {
        size_t seed = size_t(t);
        if (t == FULL or t == ST) boost::hash_combine(seed,n);
        else boost::hash_combine(seed,0);
        return seed;
    }
};

inline size_t hash_value(feat_name const& fn) { return fn.hash_value(); }
inline bool operator != (feat_name const& f1, feat_name const& f2)
{
    return !(f1 == f2);
}

typedef sbmt::oa_hash_map<feat_name,double> feat_table;

inline std::string esccc(std::string str) {
    BOOST_FOREACH(char& c, str) {
        if (c == ':') c = 'c';
        else if (c == ',') c = 'm';
        else if (c == 'c') c = ':';
        else if (c == 'm') c = ',';
    }
    return str;
}

////////////////////////////////////////////////////////////////////////////////
//
//  pattern for source-syntax features:
//  ssyn[nt1][nt2]
//
////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
feat_table make_feat_table(Grammar& g, std::string prefix /* = "ssyn" */)
{
    boost::regex ssyn_ce("^" + prefix + "_st" + "\\[(.*)\\]\\[(.*)\\]$");
    boost::regex ssyn_re("^" + prefix + "\\[(.*)\\]\\[(.*)\\]$");
    boost::regex ssyn_bk_re("^" + prefix + "_(match|nomatch|missing)$");
    //std::cerr<< "source_tree feature table:\n";
    feat_table ft;
    boost::smatch sm;
    BOOST_FOREACH(sbmt::weight_vector::value_type v, g.get_weights()) {
        std::string fname = g.feature_names().get_token(v.first);
        if (boost::regex_match(fname,sm,ssyn_re)) {
            feat_table::key_type
                k = feat_name(feat_name::full
                             , g.dict().tag(esccc(sm.str(1)))
                             , g.dict().tag(esccc(sm.str(2))) );
            ft[k] = v.second;
            //std::cerr << sm.str(1) << ',' << sm.str(2) << ':' << v.second << '\n';
        } else if (boost::regex_match(fname,sm,ssyn_ce)) {
            feat_table::key_type
                k = feat_name(feat_name::st
                             , g.dict().tag(esccc(sm.str(1)))
                             , g.dict().tag(esccc(sm.str(2))) );
            ft[k] = v.second;
        } else if (boost::regex_match(fname,sm,ssyn_bk_re)) {
            if (sm.str(1) == "match") ft[feat_name::match] = v.second;
            else if (sm.str(1) == "nomatch") ft[feat_name::nomatch] = v.second;
            else if (sm.str(1) == "missing") ft[feat_name::missing] = v.second;
        }
    }
    //std::cerr << '\n';
    return ft;
}


struct accum {
    sbmt::score_t* result;
    feat_table const* ft;
    accum(feat_table const& ft,sbmt::score_t& result) : result(&result), ft(&ft) {}
    void operator()(std::pair<feat_name,sbmt::score_t> const& p) const
    {
        feat_table::const_iterator pos = ft->find(p.first);
        if (pos != ft->end()) {
            *result *= pow(p.second,pos->second);
        }
    }
};



template <class Output>
struct transform {
    sbmt::indexed_token_factory* dict;
    sbmt::feature_names_type* fnames;
    std::string name;
    Output* out;

    transform(sbmt::indexed_token_factory& dict, sbmt::feature_names_type& fnames, std::string name, Output& out)
    : dict(&dict), fnames(&fnames), name(name), out(&out) {}

    void operator()(std::pair<feat_name,sbmt::score_t> const& p) const
    {
        std::stringstream sstr;
        sstr << name;
        if (p.first.t == feat_name::ST) sstr << "_st";
        if (p.first.t == feat_name::FULL or p.first.t == feat_name::ST) {
            sstr << "["<<esccc(const_cast<sbmt::indexed_token_factory*>(dict)->label(p.first.get0()))
                 << "]["   <<esccc(const_cast<sbmt::indexed_token_factory*>(dict)->label(p.first.get1()))
                 << "]";
        } else if (p.first == feat_name::missing) sstr << "_missing";
        else if (p.first == feat_name::match) sstr << "_match";
        else if (p.first == feat_name::nomatch) sstr << "_nomatch";
        *(*out) = std::make_pair(const_cast<sbmt::feature_names_type*>(fnames)->get_index(sstr.str()),p.second);
        ++(*out);
    }
};

class info_factory
: public sbmt::info_factory_new_component_scores<info_factory>
{
public:
    typedef sbmt::indexed_token info_type;
    typedef boost::tuple<info_type,sbmt::score_t,sbmt::score_t> result_type;
    typedef gusc::single_value_generator<result_type> result_generator;
private:
    feat_table ft;
    boost::uint32_t froot_name;
    boost::uint32_t fvars_name;
    std::string fname;
    bool mo;

    template <class Range, class Output, class Grammar>
    Output scores_( Grammar const& g
                  , typename Grammar::rule_type r
                  , Range children
                  , Output o) const
    {
        if (g.rule_has_property(r,froot_name)) {
            sbmt::indexed_token eroot = g.get_syntax(r).lhs_root()->get_token();
            sbmt::indexed_token froot = g.template rule_property<sbmt::indexed_token>(r,froot_name);
            *o = std::make_pair( feat_name(feat_name::st,eroot,froot)
                               , sbmt::indicator );
            ++o;
        }
        if (g.rule_has_property(r,fvars_name)) {
            typename source_vars<Grammar>::type const &
                fv = g.template rule_property<typename source_vars<Grammar>::type>(r,fvars_name);
            size_t x = 0;
            BOOST_FOREACH( typename boost::range_value<Range>::type const& v
                         , children ) {
                if (is_native_tag(v.root())) {
                    info_type froot = *v.info();
                    info_type fvar = fv[x++];
                    if (is_native_tag(froot)) {
                        if (not mo) {
                            *o = std::make_pair( feat_name(feat_name::full,froot,fvar)
                                               , sbmt::indicator );
                            ++o;
                        }
                        if (fvar == froot) {
                            *o = std::make_pair( feat_name(feat_name::match)
                                               , sbmt::indicator );
                            ++o;
                        } else {
                            *o = std::make_pair( feat_name(feat_name::nomatch)
                                               , sbmt::indicator );
                            ++o;
                        }
                    } else {
                        *o = std::make_pair( feat_name(feat_name::missing)
                                           , sbmt::indicator );
                        ++o;
                    }
                }
            }
        } else {
            BOOST_FOREACH( typename boost::range_value<Range>::type const& v, children ) {
                if (is_native_tag(v.root())) {
                    if (is_native_tag(*v.info())) {
                        *o = std::make_pair( feat_name(feat_name::missing)
                                           , sbmt::indicator );
                        ++o;
                    }
                }
            }
        }
        return o;
    }

public:
    template <class Grammar>
    bool scoreable_rule(Grammar const& g, typename Grammar::rule_type r) const
    {
        return true;
    }

    template <class Grammar>
    sbmt::score_t rule_heuristic( Grammar const& g
                                , typename Grammar::rule_type r ) const
    {
        return 1.0;
    }

    template <class Grammar>
    std::string hash_string( Grammar const& g
                           , info_type i ) const
    {
        return g.dict().label(i);
    }

    template <class Range, class Grammar>
    result_generator
    create_info( Grammar const& g
               , typename Grammar::rule_type r
               , sbmt::span_t const&
               , Range children ) const
    {
        sbmt::score_t result;
        accum acc(ft,result);
        scores_(g,r,children,boost::make_function_output_iterator(acc));
        sbmt::indexed_token froot;
        if (g.rule_has_property(r,froot_name)) {
            froot = g.template rule_property<sbmt::indexed_token>(r,froot_name);
        }
        return boost::make_tuple(froot,result,1.0);
    }

    template <class Range, class Output, class Grammar>
    Output component_scores_old( Grammar& g
                               , typename Grammar::rule_type r
                               , sbmt::span_t const&
                               , Range children
                               , info_type
                               , Output out ) const
    {
        transform<Output> trns(g.dict(), g.feature_names(), fname, out);
        scores_(g,r,children,boost::make_function_output_iterator(trns));
        return out;
    }

    template <class Grammar>
    info_factory(Grammar& g, boost::uint32_t r, boost::uint32_t v, std::string f, bool mo)
      : ft(make_feat_table(g,f))
      , froot_name(r)
      , fvars_name(v)
      , fname(f)
      , mo(mo) {}
};


struct constructor;

struct setter {
    enum type {prefix, root, vars};
    constructor* cons;
    type t;
    bool is_set;
    setter() : cons(NULL), t(prefix), is_set(false) {}
};


struct constructor {
    std::string name;
    std::string prefix;
    std::string root;
    std::string vars;
    setter ps, rs, vs;
    bool mo;

    constructor(std::string name, std::string rootname = "", std::string varsname = "");

    void swap_self(constructor& o);

    constructor(constructor const& o);

    constructor& operator=(constructor const& o);

    sbmt::options_map get_options();

    template <class Grammar>
    sbmt::any_type_info_factory<Grammar> construct( Grammar& grammar
                                                  , sbmt::lattice_tree const& lat
                                                  , sbmt::property_map_type pmap )
    {
        //std::cerr << "root:" << root << "    vars:" << vars << '\n';
        return info_factory(grammar,pmap[root],pmap[vars],prefix,mo);
    }
    void init(sbmt::in_memory_dictionary& dict) {}
    bool set_option(std::string const& nm, std::string const& vl)
    {
        return false;
    }
};


} // namespace ssyn

# endif // SOURCE_SYNTAX__SOURCE_SYNTAX_HPP
