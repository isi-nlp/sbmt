#ifndef   SBMT_SEARCH_FILTER_BANK_HPP
#define   SBMT_SEARCH_FILTER_BANK_HPP

#include <sbmt/edge/edge.hpp>
#include <sbmt/chart/chart.hpp>
#include <sbmt/grammar/grammar.hpp>
#include <sbmt/grammar/sorted_rhs_map.hpp>
#include <sbmt/search/span_filter_interface.hpp>
#include <sbmt/search/unary_filter_interface.hpp>

#include <sbmt/hash/hash_map.hpp>
#include <boost/shared_ptr.hpp>

#include <sbmt/hash/read_write_mutex.hpp>
#include <boost/thread/mutex.hpp>

#include <sbmt/search/logging.hpp>

namespace sbmt {
    
SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(fb_domain,"filter_bank",search);

////////////////////////////////////////////////////////////////////////////////
///
/// controls how rules are applied and filters out applications
/// based on filter functions.
///
////////////////////////////////////////////////////////////////////////////////
template <class EdgeT, class GramT, class ChartT>
class filter_bank
{
public:
    static unsigned int default_max_unary_loop_count;
    typedef EdgeT edge_type;
    typedef GramT gram_type;
    typedef ChartT chart_type;
    typedef edge_equivalence<EdgeT> edge_equiv_type;
    typedef span_filter_factory<EdgeT,GramT,ChartT> span_filter_factory_t;
    typedef boost::shared_ptr<span_filter_factory_t> span_filter_factory_p;
    typedef unary_filter_factory<EdgeT,GramT> unary_filter_factory_t;
    typedef boost::shared_ptr<unary_filter_factory_t> unary_filter_factory_p;
    
    filter_bank( span_filter_factory_p span_filt_factory
               , unary_filter_factory_p unary_filt_factory
               , GramT& gram
               , concrete_edge_factory<EdgeT,GramT> &ef
               , edge_equivalence_pool<EdgeT>& epool
               , ChartT& chart 
               , span_t total_span );
           
    void apply_rules(span_t const& first_constituent, span_index_t right_boundary);
    void finalize(span_t const& s);
    edge_stats ecs_stats() const 
    { return ef.stats(); }
    
    span_filter_factory_p get_filter_factory() { return span_filt_factory; }
    unary_filter_factory_p get_unary_filter_factory() { return unary_filt_factory; }
    concrete_edge_factory<EdgeT,GramT>& get_edge_factory() { return ef; }
    ChartT& get_chart() { return chart; }
    GramT&  get_grammar() { return gram; }
    edge_equivalence_pool<EdgeT>& get_edge_pool() { return epool; }
    
private:
    void apply_toplevel_rules();
    void apply_unary_rules(span_t const& s);
    
    template <class ItrT> void 
    apply_unary_rules(ItrT begin, ItrT end, span_t const& spn);
    
    typedef span_filter_interface<EdgeT,GramT,ChartT> span_filter_type;
    typedef boost::shared_ptr<span_filter_type> span_filter_p;
    
    typedef stlext::hash_map< span_t
                            , span_filter_p
                            , boost::hash<span_t> 
                            > span_filter_map_type;
    

    GramT&                                   gram;
    concrete_edge_factory<EdgeT,GramT>&      ef;
    edge_equivalence_pool<EdgeT>&            epool;
    ChartT&                                  chart;
    span_filter_map_type                     span_filter_map;
    span_t                                   total_span;
    span_filter_factory_p span_filt_factory;
    unary_filter_factory_p unary_filt_factory;
    sorted_rhs_map<GramT>                    rhs_map;
    signature_index_map<GramT>               sig_map;
    read_write_mutex                         chart_mtx;
    boost::mutex                             filter_map_mtx;
    unsigned int                             max_unary_loop_count;
    span_filter_p& get_create(span_t const& s);
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#include <sbmt/search/impl/filter_bank.ipp>

#endif // SBMT_SEARCH_FILTER_BANK_HPP
