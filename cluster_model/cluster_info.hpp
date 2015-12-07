# include <sbmt/edge/sentence_info.hpp> // for span_string
# include <sbmt/search/force_sentence_filter.hpp> // for conversion to span_string
# include <sbmt/edge/any_info.hpp>
# include <sbmt/grammar/rule_feature_constructors.hpp>
# include <sbmt/grammar/tree_tags.hpp>
# include <sbmt/edge/constituent.hpp>
# include <boost/range.hpp>
# include <boost/enum.hpp>
# include <boost/foreach.hpp>
# include <boost/iterator/transform_iterator.hpp>
# include <boost/tr1/unordered_map.hpp>
# include <boost/thread/shared_mutex.hpp>
# include <boost/thread/locks.hpp>
# include <boost/algorithm/string/split.hpp>
# include <boost/algorithm/string/classification.hpp>

namespace sbmt {


struct cluster_info_factory 
  : sbmt::info_factory_new_component_scores<cluster_info_factory, true> 
{
    
    typedef bool info_type;
    typedef boost::tuple<
              info_type
            , sbmt::score_t
            , sbmt::score_t
            >  result_type;
            
    typedef gusc::single_value_generator<result_type> result_generator;
    typedef boost::uint32_t feature_name;
    
    template <class Grammar, class Accumulator>
    void
    create_info_data_bin( Grammar const& grammar
                        , typename Grammar::rule_type rule
                        , Accumulator& accum ) const
    {
        typedef std::pair<feature_name const, std::vector<feature_name> > fvpt;
        BOOST_FOREACH(fvpt const& fvp, name_map) {
            feature_name fn = fvp.first;
            std::vector<feature_name> const& fv = fvp.second;
            sbmt::score_t sc;
            bool found = false;
            for(size_t x = 0; x != weights.size(); ++x) {
                typename Grammar::feature_vector_type::const_iterator 
                    pos = rule->costs.find(fv[x]);
                if (pos != rule->costs.end()) {
                    if (not found) {
                        sc = weights[x] * sbmt::score_t(pos->second,sbmt::as_neglog10());
                        found = true;
                    } else {
                        sc += weights[x] * sbmt::score_t(pos->second,sbmt::as_neglog10());
                    }
                }
            }
            accum(std::make_pair(fn,sc));
        } 
        
    }
    
    struct weighted_product {
        weighted_product(sbmt::weight_vector const& v) : v(v), score(1.0){}
        sbmt::weight_vector const& v;
        sbmt::score_t score;
        void operator()(std::pair<feature_name,score_t> const& p)
        {
            score *= p.second ^ v[p.first];
        }
    };
    
    template <class Grammar, class Constituents>
    result_generator create_info( Grammar const& grammar
                                , typename Grammar::rule_type rule
                                , sbmt::span_t const&
                                , Constituents const& chldrn ) const
    {
        weighted_product accum(grammar.get_weights());   
        create_info_data_bin(grammar,rule,accum);
        return boost::make_tuple(true,accum.score,sbmt::score_t(1.0));
    }
    
    template <class Accumulator>
    struct components {
        Accumulator scores;
        components(Accumulator const& scores)
        : scores(scores) {}

        void operator()(std::pair<feature_name,score_t> const& p)
        {
            // taking advantage of the fact that ive decided to decree that
            // the output-iterator argument to component_scores is actually an
            // accumulator.
            // saves everyone from having to allocate a feature_vector just
            // to calculate some scores.
            *scores = p;
            ++scores;
        }
    };
    
    bool deterministic() const { return true; }
    
    template <class OutputIterator, class Constituents, class Grammar>
    OutputIterator
    component_scores_old( Grammar& grammar
                        , typename Grammar::rule_type rule
                        , sbmt::span_t const&
                        , Constituents const& children
                        , bool result
                        , OutputIterator out ) const
    {
        components<OutputIterator> accum(out);
        create_info_data_bin(grammar,rule,accum);
        return accum.scores;
    }
    
    template <class Grammar>
    std::string hash_string( Grammar const& grammar
                           , info_type const & val ) const
    {
        return "0";
    }
    
    template <class Grammar>
    bool scoreable_rule( Grammar const& grammar
                       , typename Grammar::rule_type rule ) const
    {
        return true;
    }
    
    template <class Grammar>
    sbmt::score_t rule_heuristic( Grammar const& grammar
                                , typename Grammar::rule_type rule ) const
    {
        weighted_product accum(grammar.get_weights());   
        create_info_data_bin(grammar,rule,accum);
        return accum.score;
    }
    
    cluster_info_factory( std::map<feature_name,std::vector<feature_name> > const& name_map
                        , std::vector<sbmt::score_t> const& weights )
    : name_map(name_map)
    , weights(weights)
    {}

private:
    std::map<feature_name,std::vector<feature_name> > name_map;
    std::vector<sbmt::score_t> weights;
};

struct cluster_constructor {

    sbmt::options_map get_options()
    {
        sbmt::options_map opts("cluster weighting:");
        opts.add_option( "cluster-features"
                       , sbmt::optvar(features)
                       , "comma separated list of feature names"
                       );
        
        return opts;
    }
    
    cluster_constructor() {}
    
    sbmt::substring_hash_match<sbmt::indexed_token> match;
    std::string features;
    std::string weights;
    
    bool set_option(std::string key, std::string value)
    {   
        using namespace sbmt;
        if (key == "cluster-weights") {
            weights = value;
            return true;
        } else {
            return false;
        }
    }
    
    void init(sbmt::in_memory_dictionary& dict) {}
    
    template <class Grammar>
    cluster_info_factory 
    construct(Grammar& grammar, sbmt::lattice_tree const&, sbmt::property_map_type pmap)
    {
        using namespace sbmt;
        std::vector<std::string> fnames;
        boost::split(fnames,features,boost::is_any_of(","));
        std::vector<boost::uint32_t> features;
        BOOST_FOREACH(std::string nm, fnames) {
            features.push_back(grammar.feature_names().get_index(nm));
        }
        std::vector<std::string> wstr;
        boost::split(wstr,weights,boost::is_any_of(","));
        std::vector<sbmt::score_t> weights;
        BOOST_FOREACH(std::string ws, wstr) {
            weights.push_back(sbmt::score_t(boost::lexical_cast<double>(ws)));
        }
        
        std::map< boost::uint32_t, std::vector<boost::uint32_t> >
            fnmap;
        BOOST_FOREACH(std::string nm, fnames) {
            for (size_t x = 0; x != weights.size(); ++x) {
                boost::uint32_t n = grammar.feature_names().get_index("mix." + nm);
                boost::uint32_t m = grammar.feature_names().get_index(nm + "." + boost::lexical_cast<std::string>(x));
                fnmap[n].push_back(m);
            }
        }
        return cluster_info_factory(fnmap,weights);
    }
};

} // namespace sbmt
