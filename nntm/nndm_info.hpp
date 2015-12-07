# ifndef NNTM__NNDM_INFO_HPP
# define NNTM__NNDM_INFO_HPP
# include <boost/tuple/tuple.hpp>
# include <boost/tuple/tuple_comparison.hpp>
# include <boost/shared_ptr.hpp>

# include <sbmt/edge/info_base.hpp>
# include <Eigen/Dense>
# include <string>
# include <vector>
# include <sbmt/token.hpp>
# include <sbmt/search/block_lattice_tree.hpp>
# include <sbmt/edge/options_map.hpp>
# include <sbmt/edge/constituent.hpp>
# include <sbmt/edge/ngram_info.hpp>
# include <sbmt/edge/any_info.hpp>
# include <gusc/generator/single_value_generator.hpp>
# include <sbmt/ngram/neural_lm.hpp>

# include <nntm/nntm_common.hpp>

# ifndef NDEBUG
# define PRINT_NNDM 1
# endif


class nndm_info : public sbmt::info_base< nndm_info >
{
public:
    enum {N = 5};
    enum {ctx_len=N-1};
    enum {NUM_BOUNDARIES = 2, HASH_OFFSET=100};
    typedef boost::int32_t target_id_type;
    typedef boost::int32_t source_id_type;
    typedef boost::tuple<target_id_type,source_id_type> lm_id_type;
    typedef lm_id_type*       iterator;
    typedef lm_id_type const* const_iterator;

private:
    
    lm_id_type lr[2][ctx_len]; 

 public:
    nndm_info()
    {
        for(unsigned j = 0; j < NUM_BOUNDARIES; ++j) {
            for(unsigned i  = 0; i < ctx_len; ++i) {
                lr[j][i]=lm_id_type();
            }
        }
    }
    
    nndm_info& operator=(nndm_info const& o)
    {
        for(unsigned j = 0; j < NUM_BOUNDARIES; ++j) {
            for(unsigned i  = 0; i < ctx_len; ++i) {
                lr[j][i]=o.lr[j][i];
            }
        }
        return *this;
    }

    bool equal_to(nndm_info const& other) const
    {
        for (unsigned int i = 0; i != ctx_len; ++i) {
            if (lr[0][i] != other.lr[0][i]) return false;
            if (lr[1][ctx_len - i - 1] != other.lr[1][ctx_len - i - 1]) return false;
        }
        return true;
    }

    std::size_t hash_value() const
    {
        const_iterator i = &lr[0][0];
        const_iterator e= i + ctx_len;
        std::size_t ret = 0;
        for(;i != e; ++i) {
            boost::hash_combine(ret,i->get<0>());
            boost::hash_combine(ret,i->get<1>());
        }
        e = (&lr[1][0]) + ctx_len;
        i = e - ctx_len;
        for(; i != e; ++i)  {
            boost::hash_combine(ret,i->get<0>());
            boost::hash_combine(ret,i->get<1>());
        }
        return ret;
    }

    lm_id_type const& operator()(unsigned j, unsigned i) const
    { return lr[j][i]; }

    lm_id_type& operator()(unsigned j, unsigned i)
    { return lr[j][i]; }
};

struct lm_phrase;

class nndm_factory {
public:
    typedef nndm_info info_type;
    typedef boost::tuple<info_type,sbmt::score_t,sbmt::score_t> result_type;
    typedef gusc::single_value_generator<result_type> result_generator;
    typedef std::vector<sbmt::constituent<info_type> > kids_type;
    
    template <class G>
    bool scoreable_rule(G const& grammar, typename G::rule_type rule) const
    {
        return true;
    }
    
    template <class G>
    sbmt::score_t
    rule_heuristic(G const& grammar, typename G::rule_type rule) const
    {
        return 1.0;
    }
    
    std::string
    hash_string(sbmt::in_memory_dictionary const& dict, info_type::lm_id_type const& info) const;
    
    std::string
    hash_string(sbmt::in_memory_dictionary const& dict, info_type const& info) const;
    
    template <class G>
    std::string
    hash_string(G const& g, info_type const& info) const 
    { 
        return hash_string(g.dict(),info); 
    }
    
    template <class ConstituentRange>
    kids_type make_kids(ConstituentRange const& rng)
    {
        return kids_type(boost::begin(rng),boost::end(rng));
    }
    
    bool deterministic() const;
    
    typedef std::vector<sbmt::lm_token<info_type::lm_id_type> > affiliation_vector;
    
    bool start_symbol(kids_type const& lmk, affiliation_vector const& lmstr) const;
    
    int lm_id(sbmt::indexed_token idx) const
    {
        return inputmap->find(idx)->second;
    }
    
    int lm_outid(int idx) const 
    {
        return outputmap->find(idx)->second; 
    }
    
    template <class Gram>
    int
    affiliation_string( typename align_data<Gram>::return_type ts_align
                      , sbmt::span_t spn
                      , kids_type const& lmk
                      , affiliation_vector::iterator beg
                      , affiliation_vector::iterator itr
                      , affiliation_vector::iterator end )
    {
        if (itr == end) return spn.right();
        affiliation_vector::iterator ret = itr;
        for (; itr != end; ++itr) {
            size_t tgt = itr - beg;
            if (itr->is_token()) {
                typename align_data<Gram>::type::const_iterator 
                    lower = std::lower_bound(ts_align.begin(),ts_align.end(),tgt,compare_target()),
                    upper = std::upper_bound(ts_align.begin(),ts_align.end(),tgt,compare_target());
                if (lower != upper) {
                    typename align_data<Gram>::type::const_iterator mid = lower + ((upper - lower) / 2);
                    size_t wd = mid->get<1>();
                    itr->get_token().get<1>() = lmk[wd].span().left();
                    assert(pmap.find(itr->get_token().get<1>()) != pmap.end());
                    assert(pmap.find(lmk[wd].span().left())->second.find(lmk[wd].root()) != 
                           pmap.find(lmk[wd].span().left())->second.end());
                } else if (itr != beg) {
                    affiliation_vector::iterator bk = itr; --bk;
                    if (bk->is_token()) {
                        itr->get_token().get<1>() = bk->get_token().get<1>();
                        //assert(pmap.find(itr->get_token().get<1>()) != pmap.end());
                    }
                    else {
                        int ctx = nndm_info::ctx_len - 1;
                        //while ((*(lmk[bk->get_index()].info()))(1,ctx) == null) --ctx;
                        itr->get_token().get<1>() = (*(lmk[bk->get_index()].info()))(1,ctx).get<1>();
                        //assert(pmap.find(itr->get_token().get<1>()) != pmap.end());
                    }
                } else {
                    itr->get_token().get<1>() = affiliation_string<Gram>(ts_align,spn,lmk,beg,++itr,end);
                    //assert(pmap.find(itr->get_token().get<1>()) != pmap.end());
                    break;
                }
            }
        }
        if (ret->is_token()) return ret->get_token().get<1>();
        else {
            int ctx = 0;
            //while ((*(lmk[bk->get_index()].info()))(0,ctx) == null) ++ctx;
            return (*(lmk[ret->get_index()].info()))(0,ctx).get<1>();
        }
    }
    void
    compute_ngrams( info_type &n
                  , sbmt::score_t &inside
                  , sbmt::score_t &heuristic
                  , bool is_toplevel
                  , affiliation_vector const& lmstr
                  , kids_type const& lmk );
    
    template <class Grammar>
    affiliation_vector affiliation_prestring(Grammar& grammar, typename Grammar::rule_type r)
    {
        affiliation_vector ret;
        BOOST_FOREACH(typename Grammar::syntax_rule_type::tree_node const& nd, grammar.get_syntax(r).lhs()) {
            if (nd.lexical()) ret.push_back(boost::make_tuple(lm_id(nd.get_token()),0));
            else if (nd.indexed()) ret.push_back(nd.index());
        }
        return ret;
    }

    template <class Grammar, class ConstituentIterator>
    result_generator
    create_info( Grammar const& grammar
               , typename Grammar::rule_type r
               , sbmt::span_t const& span
               , boost::iterator_range<ConstituentIterator> rng )
    {
        using namespace sbmt;
        kids_type lmk = make_kids(rng);
        token_type_id t = grammar.rule_lhs(r).type();        
        typename align_data<Grammar>::return_type 
            alstr = align_data<Grammar>::value(grammar,r,lmstrid);

        # if PRINT_NNDM
        std::cerr << "NNDM: RULE: " << grammar.get_syntax(r) << " align={{{";
        int tgt, src;
        BOOST_FOREACH(boost::tie(tgt,src), alstr) std::cerr << " " << tgt << "-" << src;
        std::cerr << " }}}\n";
        std::cerr << "NNDM: CONSTIT: + ";
        BOOST_FOREACH(sbmt::constituent<info_type> const& in, rng) if (in.info()){
            std::cerr << hash_string(grammar.dict(), *(in.info())) << " + ";
        }
        std::cerr << "\n";
        # endif

        affiliation_vector lmstr = affiliation_prestring(grammar,r);
        affiliation_string<Grammar>(alstr,span,lmk,lmstr.begin(),lmstr.begin(),lmstr.end());
        bool is_toplevel = t == top_token and not start_symbol(lmk,lmstr);
        result_type ret;
        compute_ngrams( boost::get<0>(ret) // frame 9
                      , boost::get<1>(ret)
                      , boost::get<2>(ret)
                      , is_toplevel
                      , lmstr
                      , lmk );
        ret.get<1>() = ret.get<1>() ^ lmwt;
        # if PRINT_NNDM
        std::cerr << "OUTPUT: " << hash_string(grammar.dict(),ret.get<0>()) << '\n';
        # endif
        return ret;
    }
    
    template <class Grammar, class ConstituentIterator, class ScoreOutputIterator>
    boost::tuple<ScoreOutputIterator,ScoreOutputIterator>
    component_scores( Grammar const& grammar
                    , typename Grammar::rule_type r
                    , sbmt::span_t const& span
                    , boost::iterator_range<ConstituentIterator> rng
                    , info_type const& result
                    , ScoreOutputIterator out
                    , ScoreOutputIterator hout )
    {
        using namespace sbmt;
        kids_type lmk = make_kids(rng);
        token_type_id t = grammar.rule_lhs(r).type();

        typename align_data<Grammar>::return_type 
            alstr = align_data<Grammar>::value(grammar,r,lmstrid);
        affiliation_vector lmstr = affiliation_prestring(grammar,r);
        affiliation_string<Grammar>(alstr,span,lmk,lmstr.begin(),lmstr.begin(),lmstr.end());
        bool is_toplevel = t == top_token and not start_symbol(lmk,lmstr);
        result_type ret;
        compute_ngrams( boost::get<0>(ret) // frame 9
                      , boost::get<1>(ret)
                      , boost::get<2>(ret)
                      , is_toplevel
                      , lmstr
                      , lmk );
        *out = std::make_pair(grammar.feature_names().get_index("nndm"), boost::get<1>(ret));
        ++out;
        return boost::make_tuple(out,hout);
    }
    
    nndm_factory(nndm_factory const& other);
    
    nndm_factory( sbmt::lattice_tree const& ltree
                , std::map<sbmt::indexed_token,int> const& inputmap
                , std::map<int,int> const& inputoutputmap
                , size_t id
                , double wt
                , boost::shared_ptr<nplm_model> lm
                , sbmt::in_memory_dictionary& dict );
    pathmap pmap;
    pathmap revpmap;
    boost::shared_ptr<nplm_model> lm;
    mutable boost::thread_specific_ptr<nplm::propagator> prop;
    mutable boost::thread_specific_ptr<lm_phrase> phrase;
    size_t lmstrid;
    double lmwt;
    info_type::lm_id_type start;
    boost::uint32_t sstart;
    info_type::lm_id_type end;
    boost::uint32_t send;
    info_type::lm_id_type null;
    std::map<sbmt::indexed_token,int> const* inputmap;
    std::map<int,int> const* outputmap;
    
    void init( sbmt::lattice_tree::node const& lnode );
};

class nndm_factory_constructor {
    std::string filename;
    boost::shared_ptr<nplm_model> model;
    std::map<sbmt::indexed_token,int> input_map;
    std::map<int,int> output_map;
    size_t native_size;
    size_t foreign_size;
    
public:
    nndm_factory_constructor() : native_size(0), foreign_size(0) {}
    void init(sbmt::in_memory_dictionary& dict);
    sbmt::options_map get_options();
    bool set_option(std::string name, std::string value);
    
    template <class G>
    sbmt::any_type_info_factory<G>
    construct(G& gram, sbmt::lattice_tree const& lat, sbmt::property_map_type pmap)
    {
        init(gram.dict());
        return sbmt::any_type_info_factory<G>(
                 nndm_factory( lat
                             , input_map
                             , output_map
                             , pmap["align"]
                             , gram.get_weights()[gram.feature_names().get_index("nndm")]
                             , model
                             , gram.dict() )
               );
    }
};

# endif // NNTM__NNDM_INFO_HPP
