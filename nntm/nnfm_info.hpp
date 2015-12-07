# ifndef NNTM__NNFM_INFO_HPP
# define NNTM__NNFM_INFO_HPP

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
# define PRINT_NNFM 1
# endif

class nnfm_info : public sbmt::info_base< nnfm_info >
{
 public:
    bool equal_to(nnfm_info const& other) const { return true; }
    std::size_t hash_value() const { return 0; }
};

class nnfm_factory {
public:
    typedef nnfm_info info_type;
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
    hash_string(sbmt::in_memory_dictionary const& dict, info_type const& info) const;
    
    template <class G>
    std::string
    hash_string(G const& g, info_type const& info) const 
    { 
        return hash_string(g.dict(),info); 
    }
    
    bool deterministic() const;

    int lm_id(sbmt::indexed_token idx) const
    {
        return inputmap->find(idx)->second;
    }
    
    int lm_outid(int idx) const 
    {
        return outputmap->find(idx)->second; 
    }
    
    void
    compute_ngrams( info_type& n
                  , sbmt::score_t& inside
                  , sbmt::score_t& heuristic
                  , std::vector< std::pair<int,int> > const& fertility );
    
    
    template <class Grammar, class ConstituentIterator>
    std::vector< std::pair<int,int> > 
    fertility( Grammar const& g
             , typename align_data<Grammar>::return_type a
             , boost::iterator_range<ConstituentIterator> rng )
    {
        int sz = 0;
        int t, s;
	BOOST_FOREACH(sbmt::constituent<nnfm_info> const& c, rng) ++sz;
        std::vector< std::pair<int,int> > f(sz);
        int x = 0;
        BOOST_FOREACH(sbmt::constituent<nnfm_info> const& c, rng) {
            f[x] = std::make_pair(c.span().left(),0);
            if (sbmt::is_nonterminal(c.root())) f[x].second = -2;
            ++x;
        }

        BOOST_FOREACH(boost::tie(t,s), a) {
            f[s].second += 1;
        }
        return f;
    }

    template <class Grammar, class ConstituentIterator>
    result_generator
    create_info( Grammar const& grammar
               , typename Grammar::rule_type r
               , sbmt::span_t const& span
               , boost::iterator_range<ConstituentIterator> rng )
    {
        using namespace sbmt;
        
        typename align_data<Grammar>::return_type 
            alstr = align_data<Grammar>::value(grammar,r,lmstrid);
        # if PRINT_NNFM
        std::cerr << "NNFM: RULE: " << grammar.get_syntax(r) << " align={{{";
        int tgt, src;
        BOOST_FOREACH(boost::tie(tgt,src), alstr) std::cerr << " " << tgt << "-" << src;
        std::cerr << " }}}\n";
        # endif
        result_type ret;
        compute_ngrams( boost::get<0>(ret) // frame 9
                      , boost::get<1>(ret)
                      , boost::get<2>(ret)
                      , fertility(grammar,alstr,rng) );
        ret.get<1>() = ret.get<1>() ^ lmwt;
        # if PRINT_NNFM
        std::cerr << "NNFM: OUTPUT: " << hash_string(grammar.dict(),ret.get<0>()) << '\n';
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

        typename align_data<Grammar>::return_type 
            alstr = align_data<Grammar>::value(grammar,r,lmstrid);

        result_type ret;
        compute_ngrams( boost::get<0>(ret) // frame 9
                      , boost::get<1>(ret)
                      , boost::get<2>(ret)
                      , fertility(grammar,alstr,rng) );
        *out = std::make_pair(grammar.feature_names().get_index("nnfm"), boost::get<1>(ret));
        ++out;
        return boost::make_tuple(out,hout);
    }
    
    sbmt::score_t prob(int spn, int fertility);
    
    nnfm_factory(nnfm_factory const& other);
    
    nnfm_factory( sbmt::lattice_tree const& ltree
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
    size_t lmstrid;
    double lmwt;
    boost::uint32_t start;
    boost::uint32_t sstart;
    boost::uint32_t end;
    boost::uint32_t send;
    boost::uint32_t null;
    std::map<sbmt::indexed_token,int> const* inputmap;
    std::map<int,int> const* outputmap;
    
    void init( sbmt::lattice_tree::node const& lnode );
};

class nnfm_factory_constructor {
    std::string filename;
    boost::shared_ptr<nplm_model> model;
    std::map<sbmt::indexed_token,int> input_map;
    std::map<int,int> output_map;
    size_t native_size;
    size_t foreign_size;
    
public:
    nnfm_factory_constructor() : native_size(0), foreign_size(0) {}
    void init(sbmt::in_memory_dictionary& dict);
    sbmt::options_map get_options();
    bool set_option(std::string name, std::string value);
    
    template <class G>
    sbmt::any_type_info_factory<G>
    construct(G& gram, sbmt::lattice_tree const& lat, sbmt::property_map_type pmap)
    {
        init(gram.dict());
        return sbmt::any_type_info_factory<G>(
                 nnfm_factory( lat
                             , input_map
                             , output_map
                             , pmap["align"]
                             , gram.get_weights()[gram.feature_names().get_index("nnfm")]
                             , model
                             , gram.dict() )
               );
    }
};

# endif // NNTM__NNFM_INFO_HPP
