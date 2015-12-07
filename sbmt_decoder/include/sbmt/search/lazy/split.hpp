# if ! defined(SBMT__SEARCH__LAZY__SPLIT_HPP)
# define       SBMT__SEARCH__LAZY__SPLIT_HPP

# include <sbmt/search/lazy/cube.hpp>
# include <sbmt/hash/ref_array.hpp>

namespace sbmt { namespace lazy {

////////////////////////////////////////////////////////////////////////////////

template <class Grammar, class RuleMap, class CellSequence>
class split_processor_result {
    typedef typename boost::range_value<CellSequence>::type cell_type;
    typedef typename boost::range_value<cell_type>::type edge_equiv_type;
    typedef typename edge_equiv_type::edge_type edge_type;
public:
    typedef edge_type type;
};

////////////////////////////////////////////////////////////////////////////////

template <class Grammar, class RuleMap, class CellSequence>
class split_processor
: public boost::iterator_facade<
  split_processor<Grammar,RuleMap,CellSequence>
, typename split_processor_result<Grammar,RuleMap,CellSequence>::type const
, std::input_iterator_tag
> {
    typedef CellSequence cell_sequence_type;
    typedef typename boost::range_value<CellSequence>::type cell_type;
    typedef typename boost::range_value<cell_type>::type edge_equiv_type;
    typedef typename edge_equiv_type::edge_type edge_type;
    typedef Grammar grammar_type;
    typedef RuleMap rule_map_type;
    typedef typename rule_map_type::value_type rule_sequence;

    typedef cube_processor<grammar_type,rule_sequence,cell_type> gen_type;
    
    struct make_cube {
        typedef gen_type result_type;
        gen_type operator()(cell_type const& c0, cell_type const& c1) const 
        {
            typename rule_map_type::const_iterator 
                pos = rules->find( cref_array( c0[0].representative().root()
                                             , c1[0].representative().root()
                                             )
                                 );
                             
            if (pos == rules->end()) {
                return gen_type();
            } else {
               return gen_type( *grammar
                              , *ef
                              , *pos
                              , c0
                              , c1
                              )
                              ;
            }
        }
        
        make_cube( rule_map_type const* rules
                 , concrete_edge_factory<edge_type,grammar_type>* ef
                 , grammar_type const* grammar
                 )
          : rules(rules)
          , ef(ef)
          , grammar(grammar) {}
    private:
        rule_map_type const* rules;
        concrete_edge_factory<edge_type,grammar_type>* ef;
        grammar_type const* grammar;
    };

    gusc::union_heap_generator<
        gusc::product_heap_generator<
            make_cube
          , gusc::peek_compare< lesser_edge_score<edge_type> >
          , cell_sequence_type
          , cell_sequence_type
        >
      , lesser_edge_score<edge_type>
    > heap;
    
    edge_type const& dereference() const { return *heap; }    
    void increment() { ++heap; }
    friend class boost::iterator_core_access;
public:
    split_processor( Grammar const& grammar
                   , concrete_edge_factory<edge_type,Grammar>& ef
                   , RuleMap const& rules
                   , CellSequence const& cells0
                   , CellSequence const& cells1 )
    : heap(
          gusc::generate_union_heap(
            gusc::generate_product_heap(
              make_cube(&rules,&ef,&grammar)
            , gusc::peek_compare< lesser_edge_score<edge_type> >()
            , cells0
            , cells1
            )
          , lesser_edge_score<edge_type>()
          )
      )
    {}
    
    typedef typename boost::result_of<gen_type()>::type result_type;
    
    operator bool() const { return bool(heap); }
    result_type operator()() { return heap(); }
};

}} // namespace sbmt::lazy

# endif //     SBMT__SEARCH__LAZY__SPLIT_HPP
