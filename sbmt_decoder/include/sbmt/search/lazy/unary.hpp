# if ! defined(SBMT__SEARCH__LAZY__UNARY_HPP)
# define       SBMT__SEARCH__LAZY__UNARY_HPP

# include <gusc/generator/peek_compare.hpp>
# include <gusc/generator/union_heap_generator.hpp>
# include <gusc/generator/product_heap_generator.hpp>
# include <gusc/generator/peekable_generator_facade.hpp>
# include <sbmt/edge/edge.hpp>
# include <sbmt/search/concrete_edge_factory.hpp>
# include <boost/utility/result_of.hpp>
# include <boost/iterator/iterator_facade.hpp>
# include <sbmt/search/lazy/cube.hpp>
# include <sbmt/search/lazy/split.hpp>

namespace sbmt { namespace lazy {

////////////////////////////////////////////////////////////////////////////////

template <class Grammar, class RuleSequence, class EdgeSequence>
class unary_square_processor 
: public gusc::peekable_generator_facade<
    unary_square_processor<Grammar,RuleSequence,EdgeSequence>
  , typename cube_processor_result<Grammar,RuleSequence,EdgeSequence>::type
  > 
    {
        typedef typename boost::range_value<EdgeSequence>::type edge_equiv_type;
        typedef typename edge_equiv_type::edge_type edge_type;
        typedef typename Grammar::rule_type rule_type;
    public:
        typedef edge_type result_type;

        unary_square_processor() {}

        unary_square_processor( Grammar const& grammar
                              , concrete_edge_factory<edge_type,Grammar>& ef
                              , RuleSequence const& rules
                              , EdgeSequence const& edges )
        : heap(
            gusc::generate_union_heap(
                gusc::generate_product_heap(
                    create_edge(&grammar,&ef)
                  , gusc::peek_compare< lesser_edge_score<edge_type> >()
                  , rules
                  , edges
                )
              , lesser_edge_score<edge_type>()
            )
          ) { }
    private:
        struct create_edge {
            typedef gusc::iterator_from_generator<
                      gusc::any_generator<edge_type> 
                    > result_type;

            create_edge( Grammar const* grammar
                       , concrete_edge_factory<edge_type,Grammar>* ef )
            : grammar(grammar)
            , ef(ef) {}
            Grammar const* grammar;
            concrete_edge_factory<edge_type,Grammar>* ef;

            result_type operator()( rule_type rule
                                  , edge_equivalence<edge_type> const& e
                                  ) const
            {
                return result_type(ef->create_edge(*grammar,rule,e));
            }
        };

        gusc::union_heap_generator<
            gusc::product_heap_generator<
                create_edge
              , gusc::peek_compare< lesser_edge_score<edge_type> >
              , RuleSequence
              , EdgeSequence
            >
          , lesser_edge_score<edge_type>
        > heap;

        bool more() const { return bool(heap); }

        void pop() { ++heap; }

        edge_type const& peek() const { return *heap; }

        friend class gusc::generator_access;
    };

////////////////////////////////////////////////////////////////////////////////

template <class Grammar, class RuleMap, class Cells>
class unary_processor
: public gusc::peekable_generator_facade<
           unary_processor<Grammar,RuleMap,Cells>
         , typename split_processor_result<Grammar,RuleMap,Cells>::type
         > 
{
    typedef typename boost::range_value<RuleMap>::type rule_sequence;
    typedef typename boost::range_value<Cells>::type edge_sequence;
    typedef 
        typename cube_processor_result<Grammar,rule_sequence,edge_sequence>::type
        edge_type;
    typedef 
        unary_square_processor<Grammar,rule_sequence,edge_sequence> 
        square_t;
    struct square_generator 
    : gusc::peekable_generator_facade<square_generator,square_t> {
        Grammar const* gram;
        RuleMap const* rmap;
        concrete_edge_factory<edge_type,Grammar>* ef;
        gusc::generator_from_range<Cells> cells;
        square_t res;
        
        square_generator( Grammar const& gram_
                        , RuleMap const& rmap_
                        , concrete_edge_factory<edge_type,Grammar>& ef_
                        , Cells cells_ )
        : gram(&gram_)
        , rmap(&rmap_)
        , ef(&ef_)
        , cells(cells_) {
            set();
        }
        
        void set() 
        {
            while (cells) {
                typename boost::range_const_iterator<RuleMap>::type 
                    pos = rmap->find(cell_root(*cells));
                if (pos != rmap->end()) {
                    res = square_t(*gram,*ef,*pos,*cells);
                    break;
                } else {
                    ++cells;
                }
            }
        }
        
        void pop() { ++cells; set(); }
        square_t const& peek() const { return res; }
        bool more() const { return bool(cells); }
    };
    
    gusc::union_heap_generator<
      square_generator
    , lesser_edge_score<edge_type>
    > heap;
    
    void pop() { ++heap; }
    bool more() const { return bool(heap); }
    edge_type const& peek() const { return *heap; }
    
    friend class gusc::generator_access;

public:
    unary_processor() {}
    unary_processor( Cells const& cells
                   , Grammar const& grammar
                   , RuleMap const& rulemap
                   , concrete_edge_factory<edge_type,Grammar>& ef)
    {
        heap = gusc::generate_union_heap( 
                 square_generator(grammar,rulemap,ef,cells)
               , lesser_edge_score<edge_type>()
               )
               ;
    }
};



}} // namespace sbmt::lazy


# endif //     SBMT__SEARCH__LAZY__UNARY_HPP
