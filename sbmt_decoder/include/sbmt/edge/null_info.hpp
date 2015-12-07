#ifndef   SBMT_EDGE_NULL_INFO_HPP
#define   SBMT_EDGE_NULL_INFO_HPP

# include <sbmt/edge/info_base.hpp>
# include <sbmt/edge/constituent.hpp>
# include <iterator>
# include <boost/static_assert.hpp>
# include <boost/range.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/type_traits.hpp>
# include <gusc/generator/single_value_generator.hpp>
# include <sbmt/edge/options_map.hpp>
# include <sbmt/search/block_lattice_tree.hpp>

# include <sbmt/feature/feature_vector.hpp>
# include <sbmt/grammar/property_construct.hpp>
namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
///  null_info is pretty much only useful for creating an edge that only 
///  considers the grammars scoring and state information for equivalence.
///  ie tm_edge := edge<null_info>
///
////////////////////////////////////////////////////////////////////////////////
class null_info : public info_base<null_info>
{
    typedef null_info self_type;
public:
    bool equal_to(null_info const& other) const { return true; }
    std::size_t hash_value() const { return 0; }
};

template <class C, class T, class TF>
void print(std::basic_ostream<C,T>&, null_info const&, TF&)
{
    return;
}

//template <class C, class T, class TF>
//void print( std::basic_ostream<C,T>& o
//          , null_info const& i
//          , TF const& tf ) {}

////////////////////////////////////////////////////////////////////////////////

template <class NullInfoType>
class null_factory
{
public:
    typedef NullInfoType info_type;
    typedef boost::tuple<info_type,score_t,score_t> result_type;
    typedef gusc::single_value_generator<result_type> result_generator;

    template <class Grammar, class ConstituentIterator>
    result_generator
    create_info( Grammar& grammar
               , typename Grammar::rule_type rule
               , span_t const& span
               , boost::iterator_range<ConstituentIterator> const& constituents
               )
    {
        BOOST_STATIC_ASSERT((
            boost::is_convertible<
                typename std::iterator_traits<ConstituentIterator>::value_type
              , constituent<null_info>
            >::value
        )); 
        return boost::make_tuple(null_info(),1.0,1.0);
    }
    
    std::vector<std::string> component_score_names() const 
    {
        return std::vector<std::string>();
    }
    
    template <class Grammar>
    std::string hash_string(Grammar const& grammar, info_type const& info) const
    { 
        return "";
    }
    
    /*
    sbmt::null_info_factory::component_scores(
        sbmt::grammar_in_mem&
      , const sbmt::detail::rule_info*&
      , const sbmt::span_t&
      , boost::iterator_range<
          boost::transform_iterator<
            sbmt::edge_info_constituent_base<sbmt::null_info>
          , gusc::iterator_from_generator<
              sbmt::constituents_generator<
                sbmt::edge_info<sbmt::null_info>
              , sbmt::get_info
              , sbmt::info_stateable
              > 
            >
          , boost::use_default
          , boost::use_default
          > 
        >
      , const sbmt::null_info&
      , boost::function_output_iterator<sbmt::multiply_accumulator<gusc::sparse_vector<unsigned int, sbmt::logmath::basic_lognumber<sbmt::logmath::exp_n<1>, float> > > >&
      , boost::function_output_iterator<sbmt::replacement<gusc::sparse_vector<unsigned int, sbmt::logmath::basic_lognumber<sbmt::logmath::exp_n<1>, float> > > >&
    )’
    */
    template <class Grammar, class ConstituentIterator, class ScoreOutputIterator, class HeurOutputIterator>
    boost::tuple<ScoreOutputIterator,HeurOutputIterator>
    component_scores( Grammar& grammar
                    , typename Grammar::rule_type rule
                    , span_t const& span
                    , boost::iterator_range<ConstituentIterator> const& constituents
                    , info_type const& result
                    , ScoreOutputIterator scores_out
                    , HeurOutputIterator heuristics_out )
    {
        BOOST_STATIC_ASSERT((
          boost::is_convertible<
            typename std::iterator_traits<ConstituentIterator>::value_type
          , constituent<info_type>
          >::value
        ));

        return boost::make_tuple(scores_out,heuristics_out);
    }
    
    template <class Grammar>
    bool scoreable_rule(Grammar& grammar, typename Grammar::rule_type rule) const
    {
        return true;
    }
    
    bool deterministic() const { return true; }
    
    template <class Grammar>
    score_t rule_heuristic(Grammar& grammar, typename Grammar::rule_type rule) const
    {
        return 1.0;
    } 
};

template <class NullInfoType>
struct null_factory_constructor
{
    options_map get_options()
    {
        return options_map("null_factory has no options");
    }
    
    // options set via options_map
    bool set_option(std::string,std::string) { return false; }
    
    void init(in_memory_dictionary& dict) {}
    
    template <class Grammar>
    null_factory<NullInfoType> 
    construct( Grammar& gram
             , lattice_tree const& lat
             , property_map_type const& pmap )
    {
        return null_factory<NullInfoType>();
    }
};

class null_info_factory : public null_factory<null_info> {};

////////////////////////////////////////////////////////////////////////////////
///
/// the only purpose for this info type is to trivially test that joined_info
/// works.  you cant declare joined_info<null_info,null_info> because every
/// base info_type must be distinct.
/// so instead, declare joined_info<null_info,null2_info>
///
////////////////////////////////////////////////////////////////////////////////
class null2_info : public null_info {};

////////////////////////////////////////////////////////////////////////////////
///
/// \see null2_info
/// 
////////////////////////////////////////////////////////////////////////////////

class null2_info_factory : public null_factory<null2_info> {};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#endif // SBMT_EDGE_NULL_INFO_HPP
