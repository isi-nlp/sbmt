# if !defined(SBMT__SEARCH__LAZY__SQUARE_HEAP_CELL_HPP)
# define      SBMT__SEARCH__LAZY__SQUARE_HEAP_CELL_HPP

# include <sbmt/search/lazy/cube.hpp>
# include <gusc/generator/peekable_generator_facade.hpp>

namespace sbmt { namespace lazy { 

template <class CellSequence>
class square_heap
: public
  gusc::peekable_generator_facade<
    square_heap<CellSequence>
  , boost::tuple<CellSequence,CellSequence>
  > {
    typedef boost::tuple<CellSequence,CellSequence> result_;
    struct less {
        typedef bool result_type;
        bool operator()(result_ const& r1, result const& r2) const
        {
            return score(r1) < score(r2);
        }
    private:
        score_t score(result_ const& r) const
        {
            return boost::get<0>(r[0].representative().score()) *
                   boost::get<1>(r[0].representative().score());
        }
    };
    struct product {
        typedef result_ result_type;
        result_ operator()(CellSequence const& cs1, CellSequence const& cs2) const
        {
            return boost::make_tuple(cs1,cs2);
        }
    };
    
    typedef gusc::product_heap_generator<product,less,CellSequence,CellSequence>
            precube_generator;
    gusc::finite_union_generator<precube_generator> heap;
    
    result_ const& peek() const { return *heap; }
    void pop() { ++heap; }
    bool more() const { return bool(heap); }
    friend class gusc::generator_access;
    
    template <class Chart>
    struct get_cell_product {
        Chart const* chart;
        typedef precube_generator result_type;
        
        template <class Partition>
        precube_generator operator()(Partition const& part) const
        {
            return precube_generator()
        }
    }
public:
    template <class Chart, class Partitions>
    square_heap(Chart const& chart, Partitions const& partitions)
     : 
};

}} // namespace sbmt::lazy

# endif    // SBMT__SEARCH__LAZY__SQUARE_HEAP_CELL_HPP
