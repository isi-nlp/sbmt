# if ! defined(SBMT__SEARCH__LAZY__PROCESS_CELL_HPP)
# define       SBMT__SEARCH__LAZY__PROCESS_CELL_HPP

# include <gusc/generator/finite_union_generator.hpp>
# include <sbmt/search/lazy/chart.hpp>
# include <sbmt/span.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/iterator/transform_iterator.hpp>
# include <boost/range.hpp>
# include <boost/utility/result_of.hpp>

namespace sbmt { namespace lazy {

struct merge_splits {
    template <class X> struct result {};
    
    template <class SplitsFactory, class Chart, class Partitions>
    struct result<merge_splits(SplitsFactory,Chart,Partitions)> {
        typedef typename chart_edge<Chart>::type Edge;
        typedef typename chart_cell_sequence<Chart>::type Cells;
        typedef typename boost::result_of<SplitsFactory(Cells,Cells)>::type Generator;
        typedef gusc::finite_union_generator<
                  Generator
                , lesser_edge_score<Edge>
                > type;
    };
    
    template <class SplitsFactory, class Chart, class Partitions>
    typename result<merge_splits(SplitsFactory,Chart,Partitions)>::type
    operator()(SplitsFactory const& sf, Chart const& chart, Partitions const& parts) const
    {
        using namespace gusc;
        return generate_finite_union(
                 boost::make_iterator_range(
                   boost::make_transform_iterator(
                     boost::begin(parts)
                   , make_split(sf,chart)
                   )
                 , boost::make_transform_iterator(
                     boost::end(parts)
                   , make_split(sf,chart)
                   )
                 )
               , lesser_edge_score<typename chart_edge<Chart>::type>()
               )
               ;
    }
private:
    template <class SplitsFactory, class Chart>
    struct make_split_ {
        SplitsFactory const* sf;
        Chart const* chart;
        make_split_(SplitsFactory const& sf, Chart const& chart) 
        : sf(&sf)
        , chart(&chart) {}
        
        typedef typename chart_cell_sequence<Chart>::type Cells;
        typedef typename boost::result_of<SplitsFactory(Cells,Cells)>::type result_type;
        
        result_type operator()(boost::tuple<span_t,span_t> const& part) const
        {
            return (*sf)((*chart)[boost::get<0>(part)], (*chart)[boost::get<1>(part)]);
        }
        
        result_type operator()(std::pair<span_t,span_t> const& part) const
        {
            return (*sf)((*chart)[part.first], (*chart)[part.second]);
        }
    };
    
    template <class SplitsFactory, class Chart>
    static make_split_<SplitsFactory,Chart> 
    make_split(SplitsFactory const& sf, Chart const& chart) 
    {
        return make_split_<SplitsFactory,Chart>(sf,chart);
    }
};


} } // namespace sbmt::lazy

# endif //     SBMT__SEARCH__LAZY__PROCESS_CELL_HPP
