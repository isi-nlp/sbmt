# if ! defined(SBMT__SEARCH__LAZY__CUBE_HPP)
# define       SBMT__SEARCH__LAZY__CUBE_HPP

# define SBMT_SEARCH_LAZY_BUCHSE 1
# define SBMT_SEARCH_LAZY_HIERO 2
# define SBMT_SEARCH_LAZY_COMB 3
# define SBMT_SEARCH_LAZY_PRODUCT_HEAP_TYPE 1

# include <gusc/generator/peek_compare.hpp>
# include <gusc/generator/union_heap_generator.hpp>
# include <gusc/generator/buchse_product_generator.hpp>
# include <gusc/generator/hiero_product_generator.hpp>
# include <gusc/generator/product_heap_generator.hpp>
# include <gusc/generator/peekable_generator_facade.hpp>
# include <sbmt/edge/edge.hpp>
# include <sbmt/search/concrete_edge_factory.hpp>
# include <boost/utility/result_of.hpp>
# include <boost/iterator/iterator_facade.hpp>

namespace sbmt { namespace lazy {

////////////////////////////////////////////////////////////////////////////////

template <class Grammar, class RuleSequence, class EdgeSequence>
class cube_processor_result {
    typedef typename boost::range_value<EdgeSequence>::type edge_equiv_type;
    typedef typename edge_equiv_type::edge_type edge_type;
public:
    typedef edge_type type;
};

////////////////////////////////////////////////////////////////////////////////

template <class Grammar, class RuleSequence, class EdgeSequence>
class cube_processor 
: public gusc::peekable_generator_facade<
    cube_processor<Grammar,RuleSequence,EdgeSequence>
  , typename cube_processor_result<Grammar,RuleSequence,EdgeSequence>::type
  > 
{
    typedef typename boost::range_value<EdgeSequence>::type edge_equiv_type;
    typedef typename edge_equiv_type::edge_type edge_type;
    typedef typename Grammar::rule_type rule_type;
    struct create_edge {
        typedef gusc::iterator_from_generator<gusc::any_generator<edge_type> >
                result_type;
        
        create_edge( Grammar const* grammar
                   , concrete_edge_factory<edge_type,Grammar>* ef )
        : grammar(grammar)
        , ef(ef) {}
        Grammar const* grammar;
        concrete_edge_factory<edge_type,Grammar>* ef;
        
        result_type operator()( rule_type rule
                              , edge_equivalence<edge_type> const& e1
                              , edge_equivalence<edge_type> const& e2
                              ) const
        {
            return result_type(ef->create_edge(*grammar,rule,e1,e2));
        }
    };
    # if SBMT_SEARCH_LAZY_PRODUCT_HEAP_TYPE==1
    typedef gusc::buchse_product_generator<
        create_edge
      , gusc::peek_compare< lesser_edge_score<edge_type> >
      , RuleSequence
      , EdgeSequence
      , EdgeSequence
    > product_type;
    # elif SBMT_SEARCH_LAZY_PRODUCT_HEAP_TYPE==2
    typedef gusc::hiero_product_generator<
        create_edge
      , gusc::peek_compare< lesser_edge_score<edge_type> >
      , RuleSequence
      , EdgeSequence
      , EdgeSequence
    > product_type;
    # elif SBMT_SEARCH_LAZY_PRODUCT_HEAP_TYPE==3
    typedef gusc::product_heap_generator<
        create_edge
      , gusc::peek_compare< lesser_edge_score<edge_type> >
      , RuleSequence
      , EdgeSequence
      , EdgeSequence
    > product_type;
    # endif
    
public:
    typedef edge_type result_type;

    cube_processor() {}
    
    cube_processor( Grammar const& grammar
                  , concrete_edge_factory<edge_type,Grammar>& ef
                  , RuleSequence const& rules
                  , EdgeSequence const& edges1
                  , EdgeSequence const& edges2 )
    : heap(
        gusc::generate_union_heap(
        # if SBMT_SEARCH_LAZY_PRODUCT_HEAP_TYPE==1
            gusc::generate_buchse_product(
        # elif SBMT_SEARCH_LAZY_PRODUCT_HEAP_TYPE==2
            gusc::generate_hiero_product(
        # elif SBMT_SEARCH_LAZY_PRODUCT_HEAP_TYPE==3
            gusc::generate_product_heap(
        # endif
                create_edge(&grammar,&ef)
              , gusc::peek_compare< lesser_edge_score<edge_type> >()
              , rules
              , edges1
              , edges2
            )
          , lesser_edge_score<edge_type>()
        )
      ) 
      { 
          assert(boost::begin(rules) != boost::end(rules));
          assert(boost::begin(edges1) != boost::end(edges1));
          assert(boost::begin(edges2) != boost::end(edges2));
          assert(grammar.rule_rhs(rules[0],0) == edges1[0].representative().root());
          assert(grammar.rule_rhs(rules[0],1) == edges2[0].representative().root());
      }
private:    
    gusc::union_heap_generator<
      product_type
    , lesser_edge_score<edge_type>
    > heap;
    
    bool more() const { return bool(heap); }
    
    void pop() { ++heap; }
    
    edge_type const& peek() const { return *heap; }
    
    friend class gusc::generator_access;
};

////////////////////////////////////////////////////////////////////////////////


}} // namespace sbmt::lazy


# endif //     SBMT__SEARCH__LAZY__CUBE_HPP
