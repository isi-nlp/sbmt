#if 0
#ifndef   SBMT_SEARCH_OLD_DECODER_FILTER_HPP
#define   SBMT_SEARCH_OLD_DECODER_FILTER_HPP

#include <sbmt/search/cell_filter.hpp>
#include <sbmt/search/span_filter.hpp>
#include <sbmt/search/cell_span_histogram.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
///  this represents my understanding of the basic components of the old 
///  decoders span-cell-filtering.  it may be missing some things, and may
///  not be most efficient way of representing it (for instance, cell beaming
///  could happen at the same time as cell/span histograming).
///
///      +---------------+---------------+
///      | +-----------+ | +-----------+ |
///      | | +-------+ | | | +-------+ | |
///      | | | tag   | | | | | virt  | | |
///      | | | thresh| | | | | thresh| | |
///      | | | span  | | | | | span  | | |
///      | | +-------+ | | | +-------+ | |
///      | | tag-cell  | | | virt-cell | |
///      | | thresh    | | | thresh    | |
///      | +-----------+ | +-----------+ |
///      | tag-cell-span | virt-cell-span|
///      | histogram     | histogram     |
///      +---------------+---------------+
///
////////////////////////////////////////////////////////////////////////////////
template <class EdgeT, class GramT, class ChartT>
class old_decoder_filter_factory
: public span_filter_factory<EdgeT,GramT,ChartT>
{
public:
    typedef span_filter_interface<EdgeT,GramT,ChartT>       filter_t;
    typedef separate_bins_factory<EdgeT,GramT,ChartT>       separate_bins_t;
    typedef span_filter_factory<EdgeT,GramT,ChartT>         base_t;
    typedef exhaustive_span_factory<EdgeT,GramT,ChartT>     span_filt_f;
    typedef cell_bin_factory<EdgeT,GramT,ChartT>            cell_filt_f;
    typedef cell_span_histogram_factory<EdgeT,GramT,ChartT> histogram_f;
    typedef boost::shared_ptr<base_t>                       ptr_t;
    typedef ratio_predicate                                 predicate_t;

    virtual void print_settings(std::ostream &o) const 
    {
        o << "old_decoder_filter_factory{ ";
        factory->print_settings(o);
        o << " }";
    }
    
    virtual bool adjust_for_retry(unsigned i) 
    {
        return factory->adjust_for_retry(i);
    }
    
    old_decoder_filter_factory( score_t      tag_span_thresh
                              , std::size_t  tag_span_max
                              , score_t      virt_span_thresh
                              , std::size_t  virt_span_max
                              , score_t      tag_cell_thresh
                              , std::size_t  tag_cell_max
                              , score_t      virt_cell_thresh
                              , std::size_t  virt_cell_max
                              , span_t       target_span
                              )
    : base_t(target_span)
    {        
        ptr_t virt_span_filt_f(new span_filt_f( predicate_t(virt_span_thresh)
                                              , target_span ));
                                              
        ptr_t tag_span_filt_f(new span_filt_f( predicate_t(tag_span_thresh)
                                             , target_span ));
    
        ptr_t virt_cell_filt_f(new cell_filt_f( virt_span_filt_f
                                              , predicate_t(virt_cell_thresh)
                                              , target_span ));
                                              
        ptr_t tag_cell_filt_f(new cell_filt_f( tag_span_filt_f
                                             , predicate_t(tag_cell_thresh)
                                             , target_span ));
                                             
        ptr_t virt_hist_f(new histogram_f( virt_cell_filt_f
                                         , virt_span_max
                                         , virt_cell_max
                                         , target_span ));

        ptr_t tag_hist_f(new histogram_f( tag_cell_filt_f
                                        , tag_span_max
                                        , tag_cell_max
                                        , target_span ));            
                                        
        factory.reset(new separate_bins_t(tag_hist_f,virt_hist_f,target_span));                             
    }
    
    typename base_t::result_type virtual
    create( span_t const& target
          , GramT& gram
          , concrete_edge_factory<EdgeT,GramT>& ecs
          , ChartT& chart ) { return factory->create(target,gram,ecs,chart); }

    virtual ~old_decoder_filter_factory(){}

private:
    ptr_t factory;
};

} // namespace sbmt

#endif // SBMT_SEARCH_OLD_DECODER_FILTER_HPP
#endif // 0