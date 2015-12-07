# ifndef   SBMT__SRL__AGREEMENT__INFO_HPP
# define   SBMT__SRL__AGREEMENT__INFO_HPP

# include <sbmt/token.hpp>
# include <sbmt/logmath.hpp>
# include <sbmt/edge/info_base.hpp>
# include <sbmt/edge/any_info.hpp>
# include <sbmt/logging.hpp>
# include <sbmt/span.hpp>
# include <gusc/generator/single_value_generator.hpp>
# include <string>
# include <boost/functional/hash.hpp>
# include <boost/regex.hpp>
# include <cfloat>
# include <tr1/unordered_set>
# include <set>
# include <map>

namespace srl {

struct feature {
    enum type { match = 0, revmatch = 1, nomatch = 2 };
    type t;
    sbmt::indexed_token n;
    feature(sbmt::indexed_token n, type t) : n(n), t(t) {}
};

inline bool operator<(feature const& f1, feature const& f2)
{
    return (f1.n < f2.n) or (f1.n == f2.n and f1.t < f2.t);
}

inline bool operator==(feature const& f1, feature const& f2)
{
    return (f1.n == f2.n) and (f1.t == f2.t);
}

inline std::ostream& operator << (std::ostream& out, feature const& f)
{
    std::string n;
    if (f.t == feature::match) n = "agreement";
    else if (f.t == feature::revmatch) n = "revgreement";
    else if (f.t == feature::nomatch) n = "nogreement";
    return out << n << "[" << f.n << "]";
}

typedef gusc::sparse_vector<feature,double> weight_vector;

std::string 
feat2str(feature const& f, sbmt::indexed_token_factory& dict);

std::pair<bool,feature> 
str2feat(std::string const& t, sbmt::indexed_token_factory& dict);

template <class Grammar> struct headmarker {};

template <class Grammar>
struct align_data {
    typedef std::vector< boost::tuple<boost::uint8_t,boost::uint8_t> > type;
    typedef type const& return_type;
    static return_type value( Grammar const& grammar
                            , typename Grammar::rule_type r
                            , size_t lmstrid )
    {
        return grammar.template rule_property<type>(r,lmstrid);
    }
};


typedef std::multimap< 
          int
        , sbmt::indexed_token
        > pathmap;

typedef std::set<sbmt::indexed_token> label_map;

typedef std::map< 
          boost::tuple<int,int>
        , sbmt::indexed_token
        > incidence_set;

typedef std::map<int,incidence_set> sentence_incidence_map;

typedef std::map<
          sbmt::indexed_token
        , std::map<sbmt::indexed_token,float>
        > alignment_score_map;

float alignment_score( alignment_score_map const& asmap
                     , sbmt::indexed_token tgt
                     , sbmt::indexed_token src );

////////////////////////////////////////////////////////////////////////////////

struct compare_target {
    bool operator()( boost::tuple<boost::uint8_t,boost::uint8_t> b 
                   , boost::uint8_t c ) const
    {
        return b.get<0>() < c;
    }
    bool operator()( boost::uint8_t c
                   , boost::tuple<boost::uint8_t,boost::uint8_t> b ) const
    {
        return c < b.get<0>();
    }
};

////////////////////////////////////////////////////////////////////////////////

class info : public sbmt::info_base<info> {
public:
    sbmt::indexed_token target_word;
    int32_t source_position;
    info( sbmt::indexed_token const& tgt = sbmt::indexed_token()
        , int32_t src = -1 ) 
    : target_word(tgt)
    , source_position(src) {}
    
    bool equal_to(info const& other) const
    {
        return target_word == other.target_word and 
               source_position == other.source_position;
    }
    
    size_t hash_value() const
    {
        size_t seed(0);
        boost::hash_combine(seed,target_word);
        boost::hash_combine(seed,source_position);
        return seed;
    }
};

////////////////////////////////////////////////////////////////////////////////

class info_factory {
public:
    typedef info info_type;
    typedef boost::tuple<info_type,sbmt::score_t,sbmt::score_t> result_type;
    typedef gusc::single_value_generator<result_type> result_generator;
private:
    typedef std::vector<info_type> affiliation_vector;
    template <class Grammar>
    affiliation_vector affiliation_prestring( Grammar& grammar
                                            , typename Grammar::rule_type r
                                            , info_type const* children ) const
    {
        affiliation_vector ret;
        BOOST_FOREACH( typename Grammar::syntax_rule_type::tree_node const& nd
                     , grammar.get_syntax(r).lhs() ) {
            if (nd.indexed()) ret.push_back(children[nd.index()]);
            else ret.push_back(info(nd.get_token(),-1));
        }
        return ret;
    }
    
    template <class Gram>
    void
    affiliation_string( typename align_data<Gram>::return_type ts_align
                      , info const* children
                      , affiliation_vector::iterator beg
                      , affiliation_vector::iterator end ) const
    {
        affiliation_vector::iterator itr = beg;
        size_t tgt = 0;
        for (; itr != end; ++itr) {
            if (is_lexical(itr->target_word)) { // a word, not a variable
                sbmt::indexed_token tgtwd = itr->target_word;
                typename align_data<Gram>::type::const_iterator 
                    lower = std::lower_bound(ts_align.begin(),ts_align.end(),tgt,compare_target()),
                    upper = std::upper_bound(ts_align.begin(),ts_align.end(),tgt,compare_target());
                float scr = -1;
                for (;lower != upper; ++lower) {
                    size_t wdpos = lower->get<1>();
                    sbmt::indexed_token wd;
                    BOOST_FOREACH(boost::tie(wdpos,wd),pmap.equal_range(wdpos)) {
                        float wdscr = alignment_score(*asmap,tgtwd,wd);
                        if (wdscr > scr) {
                            itr->source_position = children[wdpos].source_position;
                            scr = wdscr;
                        }
                    }
                }
                ++tgt;
            }
        }
    }
    
    template <class Grammar, class Rule, class Prop, class Accum>
    info_type
    score_info( Grammar const& g
              , Rule const& r
              , typename Rule::tree_node const& n 
              , Prop const& hposvec
              , info_type const* children
              , affiliation_vector const& af
              , Accum& accum ) const
    {
        info_type ret;
        uint32_t hpos = hposvec[&n - r.lhs_begin()];
        size_t cn = 0;
        if (n.indexed()) {
            ret = children[n.index()];
        }
        else if (n.children_begin()->lexical()) { // preterminal case
            int p = (&n + 1) - r.lhs_begin();
            ret = info_type(n.children_begin()->get_token(),af.at(p).source_position);
        } else {
            sbmt::indexed_token lbl;
            info dep, head;
            BOOST_FOREACH(typename Rule::tree_node const& cnd,n.children()) {
                ++cn;
                info_type hw;
                hw = score_info(g,r,cnd,hposvec,children,af,accum);
                if (cn == hpos) head = hw; 
                else if (lblset->find(hw.target_word) != lblset->end()) lbl = hw.target_word;
                else dep = hw;
            }
            if (lbl != sbmt::indexed_token() and head.source_position != -1 and dep.source_position != -1) {
                //std::cerr << "srl " << boost::make_tuple(head.source_position, dep.source_position, lbl) << " ";
                int hs = head.source_position, ds = dep.source_position;
                if (invlblset->find(lbl) != lblset->end()) std::swap(hs,ds);
                incidence_set::const_iterator pos = incset.find(boost::make_tuple(hs,ds));
                if (pos != incset.end()) {
                    accum(std::make_pair(feature(pos->second,feature::match),sbmt::score_t(1,sbmt::as_neglog10())));
                } else if (record_nomatch) { 
                    accum(std::make_pair(feature(lbl,feature::nomatch),sbmt::score_t(1,sbmt::as_neglog10())));
                }
            }
            ret = head;
        }
        return ret;
    }
    
    struct weighted_product {
        weighted_product(weight_vector const& v) : v(v), score(sbmt::as_one()){}
        weight_vector const& v;
        sbmt::score_t score;
        void operator()(std::pair<feature,sbmt::score_t> const& p)
        {
            score *= p.second ^ v[p.first];
        }
    };
    
    template <class Accumulator>
    struct convert_feature_id {
        Accumulator scores;
        sbmt::feature_dictionary& fdict;
        sbmt::in_memory_dictionary& dict;
        convert_feature_id( sbmt::feature_dictionary& fdict
                          , sbmt::in_memory_dictionary& dict
                          , Accumulator const& scores)
        : scores(scores)
        , fdict(fdict)
        , dict(dict) {}

        void operator()(std::pair<feature,sbmt::score_t> const& p)
        {
            // taking advantage of the fact that ive decided to decree that
            // the output-iterator argument to component_scores is actually an
            // accumulator.
            // saves everyone from having to allocate a feature_vector just
            // to calculate some scores.
            *scores = std::make_pair(fdict.get_index(feat2str(p.first,dict)),p.second);
            ++scores;
        }
    };
    
    template <class Grammar, class Range, class Accum>
    info_type
    score_info( Grammar const& g
              , typename Grammar::rule_type r
              , sbmt::span_t const& spn
              , Range children
              , Accum& accum ) const
    {
        typedef typename headmarker<Grammar>::type headmarker_type;
        headmarker_type const& hmark = g.template rule_property<headmarker_type>(r,headmarkerid);
        typename align_data<Grammar>::return_type alstr = align_data<Grammar>::value(g,r,alignstrid);
        info_type cvec[256];
        info_type* citr = cvec;
        BOOST_FOREACH(sbmt::constituent<info_type> const& ci,children) {
            if (sbmt::is_nonterminal(ci.root())) *citr = *ci.info();
            else *citr = info_type(ci.root(),ci.span().left());
            ++citr;
        }
        affiliation_vector af = affiliation_prestring(g,r,cvec);
        affiliation_string<Grammar>(alstr,cvec,af.begin(),af.end());

        return score_info(g,r->rule,*r->rule.lhs_root(),hmark,cvec,af,accum);
    }
    
public:
    template <class Grammar>
    bool scoreable_rule(Grammar const& g, typename Grammar::rule_type r) const
    {
        return true;
    }

    bool deterministic() const { return true; }

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
        return "";
    }
    
    template <class Grammar, class Range>
    result_generator
    create_info( Grammar const& g
               , typename Grammar::rule_type r
               , sbmt::span_t const& spn
               , Range children ) const
    {
        result_type res;
        sbmt::score_t heur;
        info_type in;
        weighted_product wp(wts);
        in = score_info(g,r,spn,children,wp);
        return boost::make_tuple(in,wp.score,heur);
    }

    template <class Grammar, class Range, class Output, class HeurOutput>
    boost::tuple<Output,HeurOutput>
    component_scores( Grammar const& g
                    , typename Grammar::rule_type r
                    , sbmt::span_t const& spn
                    , Range children
                    , info_type i
                    , Output o
                    , HeurOutput ho ) const
    {
        sbmt::score_t scr;
        convert_feature_id<Output> cfi(g.feature_names(),g.dict(),o);
        score_info(g,r,spn,children,cfi);
        return boost::make_tuple(o,ho);
    }
    
    info_factory( sbmt::in_memory_dictionary& dict
                , sbmt::feature_dictionary& fdict
                , sbmt::weight_vector const& w
                , sbmt::property_map_type propmap
                , sbmt::lattice_tree const& ltree
                , alignment_score_map const* asmap
                , label_map const* lblset
                , label_map const* invlblset
                , incidence_set incset
                , bool match_label
                , bool record_nomatch );
private:
    void
    init( sbmt::lattice_tree::node const& lnode
        , sbmt::in_memory_dictionary& dict );
    
    uint32_t headmarkerid;
    uint32_t alignstrid;
    weight_vector wts;
    alignment_score_map const* asmap;
    label_map const* lblset;
    label_map const* invlblset;
    incidence_set incset;
    pathmap pmap;
    bool match_label;
    bool record_nomatch;
};

////////////////////////////////////////////////////////////////////////////////

class info_constructor {
    label_map lblmap;
    label_map invlblmap;
    sentence_incidence_map incmap;
    alignment_score_map asmap;
    std::string incfile;
    std::string asfile;
    size_t tgtmax;
    bool match_label;
    bool record_nomatch;
    info_factory construct( sbmt::in_memory_dictionary& dict 
                          , sbmt::feature_dictionary& fdict
                          , sbmt::weight_vector const& weights
                          , sbmt::lattice_tree const& lat
                          , sbmt::property_map_type pmap );
public:
    info_constructor();
    void init(sbmt::in_memory_dictionary& dict);
    sbmt::options_map get_options();
    bool set_option(std::string const& nm, std::string const& vl);
    template <class Grammar>
    sbmt::any_type_info_factory<Grammar>
    construct( Grammar& grammar
             , sbmt::lattice_tree const& lat
             , sbmt::property_map_type pmap )
    {
        return construct( grammar.dict()
                        , grammar.feature_names()
                        , grammar.get_weights()
                        , lat
                        , pmap );
    }
};

////////////////////////////////////////////////////////////////////////////////

} // namespace srl
# endif // SBMT__SRL__AGREEMENT__INFO_HPP
