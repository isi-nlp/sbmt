#ifndef   SBMT_SEARCH_CELL_FILTER_HPP
#define   SBMT_SEARCH_CELL_FILTER_HPP

#include <sbmt/edge/edge.hpp>
#include <sbmt/chart/chart.hpp>
#include <sbmt/grammar/grammar.hpp>
#include <sbmt/search/edge_filter.hpp>
#include <sbmt/search/span_filter_interface.hpp>
#include <sbmt/hash/oa_hashtable.hpp>
#include <sbmt/hash/functors.hpp>

#include <utility>
#include <boost/functional/hash.hpp>
#include <sbmt/hash/hash_map.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
/// a cell_bin_filter takes a span_filter_interface, which it delegates to do 
/// all of the apply_rule work.  after finalization, each edge in the contained
/// span_filter is further restricted based on a user provided filter predicate,
/// each cell being filtered independently.
///
///         apply_edge
///              | call span_filt.apply_edge()
///              V
///        +-----------+
///        | span_filt |
///        +-----------+
/// finalize:    |  insert edges from span_filt into appropriate cell's
///              V  edge_filter
/// +--------+-------+-------+
/// | cell1  | cell2 | cell3 |
/// +--------+-------+-------+
///
/// pop, top, empty: pull edges out of cell edge_filters, one cell at a time.
///
////////////////////////////////////////////////////////////////////////////////
template <class EdgeT, class GramT, class ChartT>
class cell_bin_filter
: public span_filter_interface<EdgeT, GramT, ChartT>
{
public:  
    typedef span_filter_interface<EdgeT, GramT, ChartT> base_t;
    
    template <class FilterFunc>
    cell_bin_filter( boost::shared_ptr<base_t> span_filt
                   , FilterFunc filter_func
                   , span_t const& target_span
                   , GramT& gram
                   , concrete_edge_factory<EdgeT,GramT>& ecs
                   , ChartT& chart );
    
    virtual void finalize();
    virtual bool is_finalized() const;
    
    virtual bool empty() const;
    // note: method is inlined for MSVC compatibility
    virtual typename base_t::edge_equiv_type const& top() const { return current->second.top(); }
    virtual void pop();
    
    virtual ~cell_bin_filter() {}   
private:
    typedef edge_filter<EdgeT,bool> edge_filter_t;
    typedef stlext::hash_map< indexed_token
                               , edge_filter_t
                               , boost::hash<indexed_token> 
                               > filter_table_t;
            
    filter_table_t                                    cell_filters;
    boost::shared_ptr<base_t>                         span_filt;
    edge_filter_predicate<typename base_t::edge_type> filter_func;
    typename filter_table_t::iterator                 current;
    typename filter_table_t::iterator                 end;
};

////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class GramT, class ChartT>
class cell_bin_factory
: public span_filter_factory<EdgeT, GramT, ChartT>
{
public:
    typedef span_filter_factory<EdgeT, GramT, ChartT> base_t;
    
    template <class FilterFunc>
    cell_bin_factory( boost::shared_ptr<base_t> span_filt_f
                    , FilterFunc func
                    , span_t const& total_span )
    : base_t(total_span)
    , filter_func(make_predicate_edge_filter<EdgeT>(func))
    , span_filt_f(span_filt_f){}
    
    virtual typename base_t::result_type 
    create( span_t const& target
          , GramT& gram
          , concrete_edge_factory<EdgeT,GramT>& ecs
          , ChartT& chart )
    {
        typedef span_filter_interface<EdgeT,GramT,ChartT> span_filter_t;
        boost::shared_ptr<span_filter_t> 
            p(span_filt_f->create(target,gram,ecs,chart));
            
        return new cell_bin_filter<EdgeT,GramT,ChartT>( p
                                                      , filter_func
                                                      , target
                                                      , gram
                                                      , ecs
                                                      , chart );
    }

    template <class O>
    void print(O &o) const
    {
        o << "cell_bin_factory{ ";
        filter_func.print(o);
        o << " }";
    }
    
    virtual void print_settings(std::ostream &o) const 
    { print(o); }
    
    virtual bool adjust_for_retry(unsigned i) 
    { return filter_func.adjust_for_retry(i); }
    
private:
    edge_filter<EdgeT> filter_func;
    boost::shared_ptr<base_t> span_filt_f;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#include <sbmt/search/impl/cell_filter.ipp>

#endif // SBMT_SEARCH_CELL_FILTER_HPP



